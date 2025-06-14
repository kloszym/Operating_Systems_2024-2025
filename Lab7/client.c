#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <limits.h>
#include <sys/wait.h>
#include <sys/types.h>

mqd_t client_q_desc = (mqd_t)-1;
mqd_t server_q_desc = (mqd_t)-1;
int client_id = -1;
pid_t child_pid = -1;
char client_q_name[255];
struct mq_attr client_q_attr;

typedef struct {
    int type;
    int client_id;
    char client_q_name[255];
    char text[256];
} chat_message_t;

void cleanup_client(int sig) {
     printf("\nClient %d shutting down...\n", client_id);

    if (child_pid > 0) {
        printf("Terminating receiver process (PID %d)...\n", child_pid);
        kill(child_pid, SIGTERM);
        wait(NULL);
    }

    if (server_q_desc != (mqd_t)-1) {
        mq_close(server_q_desc);
        server_q_desc = (mqd_t)-1;
    }

    if (client_q_desc != (mqd_t)-1) {
        if (mq_close(client_q_desc) == -1) {
             perror("mq_close client queue");
        } else {
            printf("Client queue descriptor closed.\n");
        }
        client_q_desc = (mqd_t)-1;

        if (mq_unlink(client_q_name) != -1) {
            printf("Client queue '%s' unlinked.\n", client_q_name);
        }
    }
    exit(0);
}

void run_receiver_process() {
    chat_message_t rcv_msg;
    ssize_t bytes_read;

    signal(SIGINT, SIG_IGN);
    signal(SIGTERM, SIG_DFL);

    printf("[Receiver PID %d] Started. Waiting for broadcast messages...\n", getpid());

    while(1) {
        bytes_read = mq_receive(client_q_desc, (char*)&rcv_msg, client_q_attr.mq_msgsize, NULL);

        if (bytes_read == -1) {
             printf("Receiver reciving error");
             break;
        }

        if (bytes_read >= sizeof(int) && rcv_msg.type == 4) {
            printf("\r\033[K");
            printf("ID %d: %s\n", rcv_msg.client_id, rcv_msg.text);
            printf("Enter message: ");
            fflush(stdout);
        } else {
            printf("[Receiver] Received unexpected message type %d or size %zd\n", rcv_msg.type, bytes_read);
        }
    }
    printf("[Receiver PID %d] Exiting.\n", getpid());
    exit(0);
}


int main() {
    chat_message_t snd_msg;
    chat_message_t rcv_msg;
    char input_buf[256];

    printf("Starting client...\n");

    snprintf(client_q_name, sizeof(client_q_name), "%s%d", "/client", getpid());

    client_q_attr.mq_flags = 0;
    client_q_attr.mq_maxmsg = 10;
    client_q_attr.mq_msgsize = sizeof(chat_message_t);
    client_q_attr.mq_curmsgs = 0;

    mq_unlink(client_q_name);

    client_q_desc = mq_open(client_q_name, O_RDONLY | O_CREAT | O_EXCL, 0666, &client_q_attr);
    if (client_q_desc == (mqd_t)-1) {
        printf("Client queue couldn't create.\n");
        exit(1);
    }
    printf("Client queue '%s' created. Descriptor: %d\n", client_q_name, (int)client_q_desc);

    signal(SIGINT, cleanup_client);
    signal(SIGTERM, cleanup_client);

    server_q_desc = mq_open("/msg", O_WRONLY);
    if (server_q_desc == (mqd_t)-1) {
        printf("Is the server running?\n");
        cleanup_client(0);
        exit(1);
    }
    printf("Connected to server queue '%s'. Descriptor: %d\n", "/msg", (int)server_q_desc);

    memset(&snd_msg, 0, sizeof(snd_msg));
    snd_msg.type = 1;
    strncpy(snd_msg.client_q_name, client_q_name, 255);
    snd_msg.client_q_name[255] = '\0';

    printf("Sending INIT message to server (queue name: %s)...\n", client_q_name);
    if (mq_send(server_q_desc, (const char*)&snd_msg, sizeof(snd_msg), 0) == -1) {
        printf("Can't send INIT\n");
        cleanup_client(0);
        exit(1);
    }

    printf("Waiting for Client ID from server...\n");
    ssize_t bytes_read = mq_receive(client_q_desc, (char*)&rcv_msg, client_q_attr.mq_msgsize, NULL);

    if (bytes_read == -1) {
        printf("Receive not received\n");
        cleanup_client(0);
        exit(1);
    }

    if (bytes_read < sizeof(int) || rcv_msg.type != 2) {
        printf("Received invalid or unexpected message instead of ID response (type=%d, size=%zd).\n", rcv_msg.type, bytes_read);
        cleanup_client(0);
        exit(1);
    }

    client_id = rcv_msg.client_id;
    if (client_id <= 0) {
        printf("Received invalid client ID %d from server.\n", client_id);
        cleanup_client(0);
        exit(1);
    }
    printf("Registered with server. Assigned Client ID: %d\n", client_id);

    child_pid = fork();

    if (child_pid == 0) {
        if (server_q_desc != (mqd_t)-1) mq_close(server_q_desc);
        run_receiver_process();
    } else {
        printf("Enter message:\n");

        while (1) {
            printf("Enter message: ");

            if (fgets(input_buf, 256, stdin) == NULL) {
                if (feof(stdin)) printf("\nEOF detected. ");
                break;
            }
            input_buf[strcspn(input_buf, "\n")] = 0; // Remove newline

            if (strlen(input_buf) == 0) {
                continue;
            }

            memset(&snd_msg, 0, sizeof(snd_msg));
            snd_msg.type = 3;
            snd_msg.client_id = client_id;
            strncpy(snd_msg.text, input_buf, 255);
            snd_msg.text[255] = '\0';

            if (mq_send(server_q_desc, (const char*)&snd_msg, sizeof(snd_msg), 0) == -1) {
                printf("Server queue seems unavailable. Exiting.\n");
                break;
            }
        }

        printf("Sender process exiting...\n");
        cleanup_client(0);

    }
    return 0;
}