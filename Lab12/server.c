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
    bool registered;
} client_info_t;

client_info_t clients[MAX_CLIENTS];
int epoll_fd = -1;
int server_socket_fd = -1;
volatile sig_atomic_t server_running = 1;

char global_recv_buffer[BUFFER_SIZE];
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
    char client_ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client->addr_info.sin_addr, client_ip_str, INET_ADDRSTRLEN);
    printf("%s - Server: Removing client %s (%s:%d). Reason: %s\n",
           current_time_ts(), client->name[0] ? client->name : "[Unregistered]",
           client_ip_str, ntohs(client->addr_info.sin_port), reason);

    client->active = false;
    client->name[0] = '\0';
    client->registered = false;
}

void handle_client_registration(client_info_t* client, char* name_line, const struct sockaddr_in* client_addr, socklen_t client_addr_len) {
    name_line[strcspn(name_line, "\r\n")] = 0;
    bool name_taken = false;
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i].active && clients[i].registered && &clients[i] != client && strcmp(clients[i].name, name_line) == 0) {
            name_taken = true;
            break;
        }
    }
    if (name_taken) {
        printf("%s - Server: Client name '%s' (from %s:%d) is already taken.\n", current_time_ts(), name_line, inet_ntoa(client_addr->sin_addr), ntohs(client_addr->sin_port));
        sendto(server_socket_fd, SERVER_MSG_NAME_TAKEN, strlen(SERVER_MSG_NAME_TAKEN), 0, (struct sockaddr*)client_addr, client_addr_len);
        remove_client(client, "Name taken");
        return;
    }
    strncpy(client->name, name_line, CLIENT_NAME_MAX_LEN);
    client->name[CLIENT_NAME_MAX_LEN] = '\0';
    client->registered = true;
    client->addr_info = *client_addr;
    client->last_activity_time = time(NULL);
    printf("%s - Server: Client '%s' registered from %s:%d.\n", current_time_ts(), client->name, inet_ntoa(client->addr_info.sin_addr), ntohs(client->addr_info.sin_port));
    sendto(server_socket_fd, SERVER_MSG_REGISTERED, strlen(SERVER_MSG_REGISTERED), 0, (struct sockaddr*)&client->addr_info, sizeof(client->addr_info));
}

void process_client_message(client_info_t* client, char* line, const struct sockaddr_in* client_addr, socklen_t client_addr_len) {
    char message_to_send_buf[BUFFER_SIZE + CLIENT_NAME_MAX_LEN + 64];
    line[strcspn(line, "\r\n")] = 0;
    printf("%s - Server: Received from %s: %s\n", current_time_ts(), client->name, line);
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
        sendto(server_socket_fd, message_to_send_buf, strlen(message_to_send_buf), 0,
               (struct sockaddr*)&client->addr_info, sizeof(client->addr_info));
    } else if (strncmp(line, MSG_2ALL_CMD, strlen(MSG_2ALL_CMD)) == 0) {
        char* msg_content = line + strlen(MSG_2ALL_CMD);
        while (*msg_content == ' ') msg_content++;
        snprintf(message_to_send_buf, sizeof(message_to_send_buf), "%s [%s to ALL]: %s\n",
                 current_time_ts(), client->name, msg_content);
        for (int i = 0; i < MAX_CLIENTS; ++i) {
            if (clients[i].active && clients[i].registered && &clients[i] != client) {
                sendto(server_socket_fd, message_to_send_buf, strlen(message_to_send_buf), 0,
                       (struct sockaddr*)&clients[i].addr_info, sizeof(clients[i].addr_info));
            }
        }
    } else if (strncmp(line, MSG_2ONE_CMD, strlen(MSG_2ONE_CMD)) == 0) {
        char target_name[CLIENT_NAME_MAX_LEN + 1];
        char* msg_content;
        char* M_tmp = line + strlen(MSG_2ONE_CMD);
        while (*M_tmp == ' ') M_tmp++;
        char* space_ptr = strchr(M_tmp, ' ');
        if (space_ptr == NULL) {
             char usage_msg[] = "Usage: 2ONE <target_name> <message>\n";
             sendto(server_socket_fd, usage_msg, strlen(usage_msg), 0,
                   (struct sockaddr*)&client->addr_info, sizeof(client->addr_info));
        } else {
            size_t name_len = space_ptr - M_tmp;
            if (name_len > CLIENT_NAME_MAX_LEN) name_len = CLIENT_NAME_MAX_LEN;
            strncpy(target_name, M_tmp, name_len);
            target_name[name_len] = '\0';
            msg_content = space_ptr + 1;
            snprintf(message_to_send_buf, sizeof(message_to_send_buf), "%s [%s to %s]: %s\n",
                     current_time_ts(), client->name, target_name, msg_content);
            client_info_t* target_client = NULL;
            for (int i = 0; i < MAX_CLIENTS; ++i) {
                if (clients[i].active && clients[i].registered && strcmp(clients[i].name, target_name) == 0) {
                    target_client = &clients[i];
                    break;
                }
            }
            if (target_client) {
                sendto(server_socket_fd, message_to_send_buf, strlen(message_to_send_buf), 0,
                       (struct sockaddr*)&target_client->addr_info, sizeof(target_client->addr_info));
            } else {
                sendto(server_socket_fd, SERVER_MSG_TARGET_NOT_FOUND, strlen(SERVER_MSG_TARGET_NOT_FOUND), 0,
                       (struct sockaddr*)&client->addr_info, sizeof(client->addr_info));
            }
        }
    } else if (strcmp(line, MSG_STOP_CMD) == 0) {
        printf("%s - Server: Client %s sent STOP.\n", current_time_ts(), client->name);
        remove_client(client, "Client sent STOP");
    } else if (strcmp(line, MSG_PING_RESPONSE) == 0) {
        client->ping_sent_flag = false;
    } else {
        printf("%s - Server: Unknown command from %s: %s\n", current_time_ts(), client->name, line);
        sendto(server_socket_fd, SERVER_MSG_CMD_UNKNOWN, strlen(SERVER_MSG_CMD_UNKNOWN), 0,
               (struct sockaddr*)&client->addr_info, sizeof(client->addr_info));
    }
}


void check_client_timeouts_and_ping() {
    time_t current_time_val = time(NULL);
    char ping_msg_buf[sizeof(MSG_PING_REQUEST) + 2];
    snprintf(ping_msg_buf, sizeof(ping_msg_buf), "%s\n", MSG_PING_REQUEST);

    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i].active && clients[i].registered) {
            if (clients[i].ping_sent_flag) {
                if (current_time_val - clients[i].last_activity_time > PING_TIMEOUT) {
                    printf("%s - Server: Client %s timed out.\n", current_time_ts(), clients[i].name);
                    remove_client(&clients[i], "Ping timeout");
                }
            } else {
                if (current_time_val - clients[i].last_activity_time > PING_INTERVAL) {
                    if (sendto(server_socket_fd, ping_msg_buf, strlen(ping_msg_buf), 0,
                               (struct sockaddr*)&clients[i].addr_info, sizeof(clients[i].addr_info)) < 0) {
                        remove_client(&clients[i], "Failed to send ping");
                    } else {
                        clients[i].ping_sent_flag = true;
                        clients[i].last_activity_time = current_time_val;
                    }
                }
            }
        } else if (clients[i].active && !clients[i].registered) {
            if (current_time_val - clients[i].last_activity_time > PING_TIMEOUT * 2) {
                printf("%s - Server: Client (unregistered) at %s:%d registration timeout.\n", current_time_ts(),
                       inet_ntoa(clients[i].addr_info.sin_addr), ntohs(clients[i].addr_info.sin_port));
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

    server_socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
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

    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) { perror("epoll_create1"); close(server_socket_fd); exit(EXIT_FAILURE); }

    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = server_socket_fd;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_socket_fd, &event) == -1) {
        perror("epoll_ctl add listener"); close(server_socket_fd); close(epoll_fd); exit(EXIT_FAILURE);
    }
    printf("%s - Server: Listening on port %s (epoll, UDP)\n", current_time_ts(), port_str);

    struct epoll_event events[MAX_EVENTS];
    time_t last_ping_check_time = time(NULL);

    while (server_running) {
        int n_events = epoll_wait(epoll_fd, events, MAX_EVENTS, 1000);

        if (n_events == -1) {
            if (errno == EINTR) { if (!server_running) break; continue; }
            perror("epoll_wait"); break;
        }

        for (int i = 0; i < n_events; ++i) {
            if (events[i].data.fd == server_socket_fd) {
                if (events[i].events & EPOLLIN) {
                    struct sockaddr_in datang_client_addr;
                    socklen_t datang_client_addr_len = sizeof(datang_client_addr);
                    ssize_t bytes_received = recvfrom(server_socket_fd, global_recv_buffer, BUFFER_SIZE - 1, 0,
                                                      (struct sockaddr*)&datang_client_addr, &datang_client_addr_len);

                    if (bytes_received > 0) {
                        global_recv_buffer[bytes_received] = '\0';
                        client_info_t* client_ptr = NULL;
                        int client_slot = -1;

                        for (int j = 0; j < MAX_CLIENTS; ++j) {
                            if (clients[j].active &&
                                clients[j].addr_info.sin_addr.s_addr == datang_client_addr.sin_addr.s_addr &&
                                clients[j].addr_info.sin_port == datang_client_addr.sin_port) {
                                client_ptr = &clients[j];
                                break;
                            }
                        }

                        if (client_ptr) {
                            client_ptr->last_activity_time = time(NULL);
                            if (client_ptr->registered) {
                                process_client_message(client_ptr, global_recv_buffer, &datang_client_addr, datang_client_addr_len);
                            } else {
                                handle_client_registration(client_ptr, global_recv_buffer, &datang_client_addr, datang_client_addr_len);
                            }
                        } else {
                            for (int j = 0; j < MAX_CLIENTS; ++j) {
                                if (!clients[j].active) {
                                    client_slot = j;
                                    break;
                                }
                            }

                            if (client_slot != -1) {
                                clients[client_slot].active = true;
                                clients[client_slot].addr_info = datang_client_addr;
                                clients[client_slot].last_activity_time = time(NULL);
                                clients[client_slot].ping_sent_flag = false;
                                clients[client_slot].registered = false;
                                clients[client_slot].name[0] = '\0';
                                handle_client_registration(&clients[client_slot], global_recv_buffer, &datang_client_addr, datang_client_addr_len);
                            } else {
                                printf("%s - Server: Max clients reached. Rejecting new connection from %s:%d.\n", current_time_ts(), inet_ntoa(datang_client_addr.sin_addr), ntohs(datang_client_addr.sin_port));
                                sendto(server_socket_fd, SERVER_MSG_SERVER_FULL, strlen(SERVER_MSG_SERVER_FULL), 0,
                                       (struct sockaddr*)&datang_client_addr, datang_client_addr_len);
                            }
                        }
                    } else if (bytes_received < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
                        perror("recvfrom failed");
                    }
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
    printf("%s - Server: Cleanup (epoll, UDP)...\n", current_time_ts());
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