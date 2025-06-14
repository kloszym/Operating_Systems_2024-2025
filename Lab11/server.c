#include "common.h"
#include <sys/epoll.h>

#define MAX_EVENTS (MAX_CLIENTS + 1)

typedef struct {
    int socket_fd;
    char name[CLIENT_NAME_MAX_LEN + 1];
    bool active;
    struct sockaddr_in addr_info;
    time_t last_activity_time;
    bool ping_sent_flag;
    char read_buffer[BUFFER_SIZE];
    size_t read_buffer_pos;
    bool registered;
} client_info_t;

client_info_t clients[MAX_CLIENTS];
int epoll_fd = -1;
int server_socket_fd = -1;
volatile sig_atomic_t server_running = 1;

void cleanup_server();

void server_signal_handler(int signum) {
    if (signum == SIGINT) {
        printf("\n%s - Server: SIGINT received. Shutting down...\n", current_time_ts());
        server_running = 0;
    }
}

void remove_client(client_info_t* client, const char* reason) {
    if (!client || !client->active) {
        return;
    }
    printf("%s - Server: Removing client %s (FD %d). Reason: %s\n",
           current_time_ts(), client->name[0] ? client->name : "[Unregistered]", client->socket_fd, reason);

    if (client->socket_fd != -1) {
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client->socket_fd, NULL);
        close(client->socket_fd);
        client->socket_fd = -1;
    }
    client->active = false;
    client->name[0] = '\0';
    client->registered = false;
    client->read_buffer_pos = 0;
}

void handle_client_registration(client_info_t* client, char* name_line) {
    name_line[strcspn(name_line, "\r\n")] = 0;
    bool name_taken = false;
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i].active && clients[i].registered && &clients[i] != client && strcmp(clients[i].name, name_line) == 0) {
            name_taken = true;
            break;
        }
    }
    if (name_taken) {
        printf("%s - Server: Client name '%s' (FD %d) is already taken.\n", current_time_ts(), name_line, client->socket_fd);
        write_all(client->socket_fd, SERVER_MSG_NAME_TAKEN, strlen(SERVER_MSG_NAME_TAKEN));
        remove_client(client, "Name taken");
        return;
    }
    strncpy(client->name, name_line, CLIENT_NAME_MAX_LEN);
    client->name[CLIENT_NAME_MAX_LEN] = '\0';
    client->registered = true;
    client->last_activity_time = time(NULL);
    printf("%s - Server: Client '%s' (FD %d) registered from %s:%d.\n",
           current_time_ts(), client->name, client->socket_fd,
           inet_ntoa(client->addr_info.sin_addr), ntohs(client->addr_info.sin_port));
    write_all(client->socket_fd, SERVER_MSG_REGISTERED, strlen(SERVER_MSG_REGISTERED));
}

void process_client_message(client_info_t* client, char* line) {
    char message_to_send_buf[BUFFER_SIZE + CLIENT_NAME_MAX_LEN + 64];
    line[strcspn(line, "\r\n")] = 0;
    printf("%s - Server: Received from %s (FD %d): %s\n", current_time_ts(), client->name, client->socket_fd, line);
    client->last_activity_time = time(NULL);

    if (strcmp(line, MSG_LIST_CMD) == 0) {
        strcpy(message_to_send_buf, "Active clients: ");
        bool first = true;
        for (int i = 0; i < MAX_CLIENTS; ++i) {
            if (clients[i].active && clients[i].registered) {
                if (!first) strcat(message_to_send_buf, ", ");
                strcat(message_to_send_buf, clients[i].name);
                first = false;
            }
        }
        if (first) strcpy(message_to_send_buf, "No other active clients.");
        strcat(message_to_send_buf, "\n");
        write_all(client->socket_fd, message_to_send_buf, strlen(message_to_send_buf));
    } else if (strncmp(line, MSG_2ALL_CMD, strlen(MSG_2ALL_CMD)) == 0) {
        char* msg_content = line + strlen(MSG_2ALL_CMD);
        while (*msg_content == ' ') msg_content++;
        snprintf(message_to_send_buf, sizeof(message_to_send_buf), "%s [%s to ALL]: %s\n",
                 current_time_ts(), client->name, msg_content);
        for (int i = 0; i < MAX_CLIENTS; ++i) {
            if (clients[i].active && clients[i].registered && &clients[i] != client) {
                write_all(clients[i].socket_fd, message_to_send_buf, strlen(message_to_send_buf));
            }
        }
    } else if (strncmp(line, MSG_2ONE_CMD, strlen(MSG_2ONE_CMD)) == 0) {
        char target_name[CLIENT_NAME_MAX_LEN + 1];
        char* msg_content;
        char* M_tmp = line + strlen(MSG_2ONE_CMD);
        while (*M_tmp == ' ') M_tmp++;
        char* space_ptr = strchr(M_tmp, ' ');
        if (space_ptr == NULL) {
            write_all(client->socket_fd, "Usage: 2ONE <target_name> <message>\n", strlen("Usage: 2ONE <target_name> <message>\n"));
        } else {
            size_t name_len = space_ptr - M_tmp;
            if (name_len > CLIENT_NAME_MAX_LEN) name_len = CLIENT_NAME_MAX_LEN;
            strncpy(target_name, M_tmp, name_len);
            target_name[name_len] = '\0';
            msg_content = space_ptr + 1;
            snprintf(message_to_send_buf, sizeof(message_to_send_buf), "%s [%s to %s]: %s\n",
                     current_time_ts(), client->name, target_name, msg_content);
            int target_fd = -1;
            for (int i = 0; i < MAX_CLIENTS; ++i) {
                if (clients[i].active && clients[i].registered && strcmp(clients[i].name, target_name) == 0) {
                    target_fd = clients[i].socket_fd;
                    break;
                }
            }
            if (target_fd != -1) {
                write_all(target_fd, message_to_send_buf, strlen(message_to_send_buf));
            } else {
                write_all(client->socket_fd, SERVER_MSG_TARGET_NOT_FOUND, strlen(SERVER_MSG_TARGET_NOT_FOUND));
            }
        }
    } else if (strcmp(line, MSG_STOP_CMD) == 0) {
        printf("%s - Server: Client %s (FD %d) sent STOP.\n", current_time_ts(), client->name, client->socket_fd);
        remove_client(client, "Client sent STOP");
    } else if (strcmp(line, MSG_PING_RESPONSE) == 0) {
        client->ping_sent_flag = false;
    } else {
        printf("%s - Server: Unknown command from %s (FD %d): %s\n", current_time_ts(), client->name, client->socket_fd, line);
        write_all(client->socket_fd, SERVER_MSG_CMD_UNKNOWN, strlen(SERVER_MSG_CMD_UNKNOWN));
    }
}

void handle_client_read(client_info_t* client) {
    ssize_t bytes_read;
    bytes_read = read(client->socket_fd,
                      client->read_buffer + client->read_buffer_pos,
                      BUFFER_SIZE - 1 - client->read_buffer_pos);

    if (bytes_read == -1) {
        perror("read from client failed");
        remove_client(client, "Read error");
        return;
    }
    if (bytes_read == 0) {
        remove_client(client, "Client disconnected");
        return;
    }
    client->read_buffer_pos += bytes_read;
    client->read_buffer[client->read_buffer_pos] = '\0';

    char* line_start = client->read_buffer;
    char* newline_ptr;
    while ((newline_ptr = strchr(line_start, '\n')) != NULL) {
        *newline_ptr = '\0';
        if (!client->registered) {
            handle_client_registration(client, line_start);
            if (!client->active) return;
        } else {
            process_client_message(client, line_start);
            if (!client->active) return;
        }
        line_start = newline_ptr + 1;
    }
    if (line_start < client->read_buffer + client->read_buffer_pos) {
        size_t remaining = client->read_buffer + client->read_buffer_pos - line_start;
        memmove(client->read_buffer, line_start, remaining);
        client->read_buffer_pos = remaining;
        client->read_buffer[client->read_buffer_pos] = '\0';
    } else {
        client->read_buffer_pos = 0;
    }
    if (client->read_buffer_pos >= BUFFER_SIZE - 1) {
        fprintf(stderr, "%s - Server: Client %s (FD %d) buffer nearly full. Disconnecting.\n", current_time_ts(), client->name[0] ? client->name : "[Unreg]", client->socket_fd);
        remove_client(client, "Buffer overflow / line too long");
        return;
    }
}

void handle_new_connection() {
    struct sockaddr_in client_addr_info;
    socklen_t client_addr_len = sizeof(client_addr_info);
    int client_socket_fd = accept(server_socket_fd, (struct sockaddr*)&client_addr_info, &client_addr_len);

    if (client_socket_fd == -1) {
        if (server_running) {
             perror("accept failed");
        }
        return;
    }
    int client_idx = -1;
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (!clients[i].active) {
            client_idx = i;
            break;
        }
    }
    if (client_idx == -1) {
        printf("%s - Server: Max clients reached. Rejecting.\n", current_time_ts());
        write_all(client_socket_fd, SERVER_MSG_SERVER_FULL, strlen(SERVER_MSG_SERVER_FULL));
        close(client_socket_fd);
        return;
    }
    clients[client_idx].socket_fd = client_socket_fd;
    clients[client_idx].active = true;
    clients[client_idx].addr_info = client_addr_info;
    clients[client_idx].last_activity_time = time(NULL);
    clients[client_idx].ping_sent_flag = false;
    clients[client_idx].read_buffer_pos = 0;
    clients[client_idx].registered = false;
    clients[client_idx].name[0] = '\0';

    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.ptr = &clients[client_idx];

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_socket_fd, &event) == -1) {
        perror("epoll_ctl EPOLL_CTL_ADD failed for client");
        close(client_socket_fd);
        clients[client_idx].active = false;
        return;
    }
    printf("%s - Server: Accepted new connection (FD %d, slot %d).\n", current_time_ts(), client_socket_fd, client_idx);
}

void check_client_timeouts_and_ping() {
    time_t current_time_val = time(NULL);
    char ping_msg_buf[sizeof(MSG_PING_REQUEST) + 2];
    snprintf(ping_msg_buf, sizeof(ping_msg_buf), "%s\n", MSG_PING_REQUEST);

    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i].active && clients[i].registered) {
            if (clients[i].ping_sent_flag) {
                if (current_time_val - clients[i].last_activity_time > PING_TIMEOUT) {
                    printf("%s - Server: Client %s (FD %d) timed out.\n", current_time_ts(), clients[i].name, clients[i].socket_fd);
                    remove_client(&clients[i], "Ping timeout");
                }
            } else {
                if (current_time_val - clients[i].last_activity_time > PING_INTERVAL) {
                    if (write_all(clients[i].socket_fd, ping_msg_buf, strlen(ping_msg_buf)) < 0) {
                        remove_client(&clients[i], "Failed to send ping");
                    } else {
                        clients[i].ping_sent_flag = true;
                    }
                }
            }
        } else if (clients[i].active && !clients[i].registered) {
            if (current_time_val - clients[i].last_activity_time > PING_TIMEOUT * 2) {
                printf("%s - Server: Client (FD %d) registration timeout.\n", current_time_ts(), clients[i].socket_fd);
                remove_client(&clients[i], "Registration timeout");
            }
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    char* port_str = argv[1];
    int port = atoi(port_str);
    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Invalid port number: %s\n", port_str);
        exit(EXIT_FAILURE);
    }

    setlocale(LC_ALL, "Polish");
    struct sigaction sa;
    sa.sa_handler = server_signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction"); exit(EXIT_FAILURE);
    }

    for (int i = 0; i < MAX_CLIENTS; ++i) clients[i].active = false;

    server_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_fd == -1) { perror("socket"); exit(EXIT_FAILURE); }

    int opt = 1;
    if (setsockopt(server_socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt"); close(server_socket_fd); exit(EXIT_FAILURE);
    }
    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(port);
    if (bind(server_socket_fd, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        perror("bind"); close(server_socket_fd); exit(EXIT_FAILURE);
    }
    if (listen(server_socket_fd, SOMAXCONN) < 0) {
        perror("listen"); close(server_socket_fd); exit(EXIT_FAILURE);
    }

    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) { perror("epoll_create1"); close(server_socket_fd); exit(EXIT_FAILURE); }

    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.ptr = (void*)(intptr_t)server_socket_fd;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_socket_fd, &event) == -1) {
        perror("epoll_ctl add listener"); close(server_socket_fd); close(epoll_fd); exit(EXIT_FAILURE);
    }
    printf("%s - Server: Listening on port %s (epoll, blocking sockets)\n", current_time_ts(), port_str);

    struct epoll_event events[MAX_EVENTS];
    time_t last_ping_check_time = time(NULL);

    while (server_running) {
        int n_events = epoll_wait(epoll_fd, events, MAX_EVENTS, 1000);

        if (n_events == -1) {
            if (errno == EINTR) { if (!server_running) break; continue; }
            perror("epoll_wait"); break;
        }

        for (int i = 0; i < n_events; ++i) {
            if ((intptr_t)events[i].data.ptr == server_socket_fd) {
                handle_new_connection();
            } else {
                client_info_t* client = (client_info_t*)events[i].data.ptr;
                if (!client || !client->active) continue;

                if (events[i].events & (EPOLLERR | EPOLLHUP)) {
                    remove_client(client, "EPOLLERR or EPOLLHUP");
                } else if (events[i].events & EPOLLIN) {
                    handle_client_read(client);
                }
            }
        }

        time_t current_time_val = time(NULL);
        if (current_time_val - last_ping_check_time >= 20) {
            check_client_timeouts_and_ping();
            last_ping_check_time = current_time_val;
        }
    }

    cleanup_server();
    printf("%s - Server: Shutdown complete.\n", current_time_ts());
    return 0;
}

void cleanup_server() {
    printf("%s - Server: Cleanup (epoll, blocking sockets)...\n", current_time_ts());
    server_running = 0;
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i].active) {
            remove_client(&clients[i], "Server shutdown");
        }
    }
    if (server_socket_fd != -1) {
        close(server_socket_fd);
        server_socket_fd = -1;
    }
    if (epoll_fd != -1) {
        close(epoll_fd);
        epoll_fd = -1;
    }
}