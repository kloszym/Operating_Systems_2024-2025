#include "common.h"

char client_name[CLIENT_NAME_MAX_LEN + 1];
int server_socket_fd = -1;
volatile sig_atomic_t client_running = 1;
pthread_t receiver_thread_id = 0;

void client_signal_handler(int signum) {
    if (signum == SIGINT) {
        printf("\n%s - Client: SIGINT received. Exiting...\n", current_time_ts());
        client_running = 0;
        if (server_socket_fd != -1) {
            shutdown(server_socket_fd, SHUT_RDWR);
        }
    }
}

void* receiver_thread_func(void* arg) {
    (void)arg;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;

    while (client_running) {
        bytes_received = read_line(server_socket_fd, buffer, BUFFER_SIZE);
        if (!client_running) break;

        if (bytes_received <= 0) {
            if (bytes_received == 0 && client_running) {
                printf("%s - Client: Server disconnected.\n", current_time_ts());
            } else if (client_running) {
                if (errno != ECONNRESET && errno != EBADF) {
                    perror("Client: read_line from server failed");
                }
            }
            client_running = 0;
            break;
        }

        char temp_buffer_for_cmp[BUFFER_SIZE];
        strncpy(temp_buffer_for_cmp, buffer, sizeof(temp_buffer_for_cmp)-1);
        temp_buffer_for_cmp[sizeof(temp_buffer_for_cmp)-1] = '\0';
        temp_buffer_for_cmp[strcspn(temp_buffer_for_cmp, "\r\n")] = 0;


        if (strcmp(temp_buffer_for_cmp, MSG_PING_REQUEST) == 0) {
            char response_to_send[sizeof(MSG_PING_RESPONSE) + 2];
            snprintf(response_to_send, sizeof(response_to_send), "%s\n", MSG_PING_RESPONSE);
            if (write_all(server_socket_fd, response_to_send, strlen(response_to_send)) < 0) {
                if(client_running) perror("Client: Failed to send PING_RESPONSE");
                client_running = 0;
                break;
            }
        } else {
            printf("%s", buffer);
            fflush(stdout);
        }
    }
    printf("%s - Client: Receiver thread exiting.\n", current_time_ts());
    pthread_exit(NULL);
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <client_name> <server_ipv4> <server_port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    strncpy(client_name, argv[1], CLIENT_NAME_MAX_LEN);
    client_name[CLIENT_NAME_MAX_LEN] = '\0';
    char* server_ip_str = argv[2];
    char* server_port_str = argv[3];
    int server_port = atoi(server_port_str);

    if (server_port <= 0 || server_port > 65535) {
        fprintf(stderr, "Invalid server port: %s\n", server_port_str);
        exit(EXIT_FAILURE);
    }

    setlocale(LC_ALL, "Polish");

    struct sigaction sa;
    sa.sa_handler = client_signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("Client: sigaction failed");
        exit(EXIT_FAILURE);
    }

    server_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_fd == -1) {
        perror("Client: socket creation failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(server_port);
    if (inet_pton(AF_INET, server_ip_str, &server_address.sin_addr) <= 0) {
        perror("Client: inet_pton failed for server IP");
        close(server_socket_fd);
        exit(EXIT_FAILURE);
    }

    printf("%s - Client: Connecting to server %s:%s as %s...\n",
           current_time_ts(), server_ip_str, server_port_str, client_name);

    if (connect(server_socket_fd, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        perror("Client: connect failed");
        close(server_socket_fd);
        exit(EXIT_FAILURE);
    }
    printf("%s - Client: Connected to server. Registering...\n", current_time_ts());

    char registration_msg[CLIENT_NAME_MAX_LEN + 2];
    snprintf(registration_msg, sizeof(registration_msg), "%s\n", client_name);
    if (write_all(server_socket_fd, registration_msg, strlen(registration_msg)) < 0) {
        perror("Client: Failed to send registration name");
        close(server_socket_fd);
        exit(EXIT_FAILURE);
    }

    char server_response[BUFFER_SIZE];
    ssize_t n = read_line(server_socket_fd, server_response, sizeof(server_response));
    if (n <= 0) {
        fprintf(stderr, "%s - Client: Failed to get registration response or server disconnected.\n", current_time_ts());
        close(server_socket_fd);
        exit(EXIT_FAILURE);
    }

    char temp_response_for_cmp[BUFFER_SIZE];
    strncpy(temp_response_for_cmp, server_response, sizeof(temp_response_for_cmp)-1);
    temp_response_for_cmp[sizeof(temp_response_for_cmp)-1] = '\0';
    temp_response_for_cmp[strcspn(temp_response_for_cmp, "\r\n")] = 0;


    if (strcmp(temp_response_for_cmp, "REGISTERED") == 0) {
        printf("%s - Client: Successfully registered as '%s'.\n", current_time_ts(), client_name);
    } else {
        fprintf(stderr, "%s - Client: Registration failed: %s\n", current_time_ts(), server_response);
        close(server_socket_fd);
        exit(EXIT_FAILURE);
    }

    if (pthread_create(&receiver_thread_id, NULL, receiver_thread_func, NULL) != 0) {
        perror("Client: Failed to create receiver thread");
        close(server_socket_fd);
        exit(EXIT_FAILURE);
    }

    char input_buffer[BUFFER_SIZE];
    printf("Enter command (LIST, 2ALL <msg>, 2ONE <target> <msg>, STOP):\n> ");
    fflush(stdout);

    while (client_running && fgets(input_buffer, sizeof(input_buffer), stdin) != NULL) {
        if (!client_running) break;

        input_buffer[strcspn(input_buffer, "\r\n")] = 0;

        if (strlen(input_buffer) == 0) {
            printf("> "); fflush(stdout);
            continue;
        }

        char command_to_send[BUFFER_SIZE + 1];
        snprintf(command_to_send, sizeof(command_to_send), "%s\n", input_buffer);

        if (write_all(server_socket_fd, command_to_send, strlen(command_to_send)) < 0) {
            if (client_running) {
                perror("Client: Failed to send command to server");
            }
            client_running = 0;
            break;
        }

        if (strcmp(input_buffer, MSG_STOP_CMD) == 0) {
            client_running = 0;
            break;
        }
        printf("> "); fflush(stdout);
    }

    if (client_running) {
        client_running = 0;
        if (server_socket_fd != -1) {
            char stop_cmd_final[sizeof(MSG_STOP_CMD) + 2];
            snprintf(stop_cmd_final, sizeof(stop_cmd_final), "%s\n", MSG_STOP_CMD);
            write_all(server_socket_fd, stop_cmd_final, strlen(stop_cmd_final));
        }
    }

    if (server_socket_fd != -1) {
         shutdown(server_socket_fd, SHUT_WR);
    }

    if (receiver_thread_id != 0) {
        pthread_join(receiver_thread_id, NULL);
    }

    if (server_socket_fd != -1) {
        close(server_socket_fd);
        server_socket_fd = -1;
    }

    printf("%s - Client: Exited.\n", current_time_ts());
    return 0;
}