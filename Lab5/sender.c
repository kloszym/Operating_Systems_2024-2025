#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>


void sig_handler(int sig) {
	printf("SIGUSR1 received back\n");
	exit(0);
}


int main(int argc, char *argv[]) {

	if (argc != 3) {
		printf("Usage: ./%s <PID> <1 | 2 | 3 | 4 | 5>\n", argv[0]);
		return 1;
	}

	signal(SIGUSR1, sig_handler);

	pid_t pid = atoi(argv[1]);
	int value = atoi(argv[2]);

	union sigval data;
	data.sival_int = value;

	sigqueue(pid, SIGUSR1, data);
	pause();

}