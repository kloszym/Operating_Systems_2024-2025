#ifndef COMMON_H
#define COMMON_H

#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <locale.h>

#define MAX_CLIENTS 10
#define CLIENT_NAME_MAX_LEN 31
#define MESSAGE_MAX_LEN 255
#define BUFFER_SIZE 512

#define PING_INTERVAL 120
#define PING_TIMEOUT 60

#define MSG_REGISTER_PREFIX "REGISTER:"
#define MSG_LIST_CMD "LIST"
#define MSG_2ALL_CMD "2ALL"
#define MSG_2ONE_CMD "2ONE"
#define MSG_STOP_CMD "STOP"
#define MSG_PING_REQUEST "PING_REQUEST_SERVER"
#define MSG_PING_RESPONSE "PING_RESPONSE_CLIENT"

#define SERVER_MSG_REGISTERED "REGISTERED\n"
#define SERVER_MSG_NAME_TAKEN "NAME_TAKEN\n"
#define SERVER_MSG_SERVER_FULL "SERVER_FULL\n"
#define SERVER_MSG_TARGET_NOT_FOUND "TARGET_NOT_FOUND\n"
#define SERVER_MSG_CMD_UNKNOWN "UNKNOWN_COMMAND\n"
#define SERVER_MSG_CLIENT_LEFT_FORMAT "%s - Server: Client %s has left the chat.\n"

static inline char* current_time_ts() {
    static char bufor[64];
    time_t czas;
    time(&czas);
    struct tm czasTM_data;
    if (localtime_r(&czas, &czasTM_data) == NULL) {
        strncpy(bufor, "??:??:??", sizeof(bufor) -1);
        bufor[sizeof(bufor)-1] = '\0';
        return bufor;
    }
    if (strftime(bufor, sizeof(bufor), "%H:%M:%S", &czasTM_data) == 0) {
        strncpy(bufor, "??:??:??", sizeof(bufor) -1);
        bufor[sizeof(bufor)-1] = '\0';
        return bufor;
    }
    return bufor;
}

static inline ssize_t read_line(int fd, char *buffer, size_t max_len) {
    ssize_t total_read = 0;
    ssize_t n;
    if (max_len == 0) return 0;

    n = recv(fd, buffer, max_len - 1, 0);

    if (n > 0) {
        buffer[n] = '\0';
        total_read = n;
    } else if (n == 0) {
        return 0; 
    } else {
        if (errno == EINTR) {
            return -1;
        }
        return -1;
    }
    return total_read;
}

static inline ssize_t write_all(int fd, const char *buffer, size_t len) {
    ssize_t n;
    n = send(fd, buffer, len, 0);
        if (n < 0) {
            if (errno == EINTR) {
                return -1;
            }
            return -1;
        }
    return n;
}

#endif //COMMON_H