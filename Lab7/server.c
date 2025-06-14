#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <limits.h>
#include <sys/wait.h>

typedef struct {
    int type;
    int client_id;
    char client_q_name[255];
    char text[256];
} chat_message_t;

typedef struct {
    int client_id;
    mqd_t client_q_desc;
    char client_q_name[255];
    int active;
} client_info_t;

client_info_t clients[20];
int client_count = 0;
int next_client_id = 1;
mqd_t server_q_desc = (mqd_t)-1;

void cleanup_server(int sig) {
    printf("\nServer shutting down...\n");

    for (int i = 0; i < 20; ++i) {
        if (clients[i].active && clients[i].client_q_desc != (mqd_t)-1) {
            mq_close(clients[i].client_q_desc);
            clients[i].client_q_desc = (mqd_t)-1;
        }
    }

    if (server_q_desc != (mqd_t)-1) {
        if (mq_close(server_q_desc) == -1) {
            perror("mq_close server queue");
        }
        printf("Server queue descriptor closed.\n");
        if (mq_unlink("/msg") == -1) {
            printf("Server queue have problem unlinking.\n");
        } else {
            printf("Server queue unlinked.\n");
        }
    }
    exit(0);
}

int find_client_slot() {
    for (int i = 0; i < 20; i++) {
        if (!clients[i].active) return i;
    }
    return -1;
}

int get_client_index_by_id(int id) {
     for (int i = 0; i < 20; i++) {
        if (clients[i].active && clients[i].client_id == id) return i;
    }
    return -1;
}

int main() {
    struct mq_attr attr;
    chat_message_t rcv_msg;
    chat_message_t snd_msg;

    printf("Starting server...\n");


    for (int i = 0; i < 20; ++i) {
        clients[i].active = 0;
        clients[i].client_id = -1;
        clients[i].client_q_desc = (mqd_t)-1;
        clients[i].client_q_name[0] = '\0';
    }

    signal(SIGINT, cleanup_server);

    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = sizeof(chat_message_t);
    attr.mq_curmsgs = 0;

    mq_unlink("/msg");

    server_q_desc = mq_open("/msg", O_RDONLY | O_CREAT | O_EXCL, 0666, &attr);
    if (server_q_desc == (mqd_t)-1) {
        printf("Server can't create queue");
        exit(1);
    }
    printf("Server queue '%s' created/opened. Descriptor: %d\n", "/msg", (int)server_q_desc);

    printf("Server waiting for client messages...\n");

    while (1) {

        ssize_t bytes_read = mq_receive(server_q_desc, (char*)&rcv_msg, sizeof(chat_message_t), NULL);

        if (bytes_read == -1) {
            printf("Server can't receive message");
            continue;
        }

        switch (rcv_msg.type) {
            case 1: {
                printf("Received INIT from client queue '%s'\n", rcv_msg.client_q_name);
                int slot = find_client_slot();
                if (slot == -1) {
                    printf("Max clients reached. INIT rejected for %s.\n", rcv_msg.client_q_name);
                    continue;
                }

                mqd_t client_q_desc = mq_open(rcv_msg.client_q_name, O_WRONLY);
                if (client_q_desc == (mqd_t)-1) {
                    printf("Failed to open client queue: %s\n", rcv_msg.client_q_name);
                    continue;
                }

                clients[slot].active = 1;
                clients[slot].client_id = next_client_id++;
                clients[slot].client_q_desc = client_q_desc;
                strncpy(clients[slot].client_q_name, rcv_msg.client_q_name, 255);
                clients[slot].client_q_name[255] = '\0';
                client_count++;

                printf("Client %d connected (Queue: %s, Descriptor: %d, Slot: %d)\n",
                       clients[slot].client_id, clients[slot].client_q_name, (int)client_q_desc, slot);

                memset(&snd_msg, 0, sizeof(snd_msg));
                snd_msg.type = 2;
                snd_msg.client_id = clients[slot].client_id;

                if (mq_send(client_q_desc, (const char*)&snd_msg, sizeof(snd_msg), 0) == -1) {
                    mq_close(client_q_desc);
                    clients[slot].active = 0;
                    client_count--;
                } else {
                     printf("Sent assigned ID %d to client queue %s\n", clients[slot].client_id, clients[slot].client_q_name);
                }
                break;
            }

            case 3: {
                 int sender_idx = get_client_index_by_id(rcv_msg.client_id);
                 if (sender_idx == -1) {
                     printf("MESSAGE from unknown client ID %d\n", rcv_msg.client_id);
                     continue;
                 }

                memset(&snd_msg, 0, sizeof(snd_msg));
                snd_msg.type = 4;
                snd_msg.client_id = rcv_msg.client_id;
                strncpy(snd_msg.text, rcv_msg.text, 255);
                snd_msg.text[255] = '\0';

                printf("Broadcasting from %d to other clients...\n", rcv_msg.client_id);
                for (int i = 0; i < 20; i++) {
                    if (clients[i].active && i != sender_idx) {
                        if (mq_send(clients[i].client_q_desc, (const char*)&snd_msg, sizeof(snd_msg), 0) == -1) {
                            printf("Error sending to client %d (queue %s).\n", clients[i].client_id, clients[i].client_q_name);
                            mq_close(clients[i].client_q_desc);
                            clients[i].client_q_desc = (mqd_t)-1;
                            clients[i].active = 0;
                            client_count--;
                        } else {
                            printf(" -> Sent to client %d\n", clients[i].client_id);
                        }
                    }
                }
                break;
            }
            default:
                fprintf(stderr, "Unknown message type received: %d\n", rcv_msg.type);
                break;
        }
    }

    cleanup_server(0);
    return 0;
}