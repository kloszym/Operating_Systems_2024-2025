#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>

int signalCounter = 0;
int isMode2 = 0;

void ctrl_c_handler(int signum, siginfo_t *info, void *ptr) {
	printf("\nWciśnięto CTRL+C\n");
}

void sig_handle(int signum, siginfo_t *info, void *ptr) {
	printf("Handeling singal %d\n", signum);
	printf("Nadawca PID: %d\n\n", info->si_pid);
	isMode2 = 0;
	signalCounter++;
	switch (info->si_value.sival_int) {
		case 1:
			printf("Signals received: %d\n\n", signalCounter);
			break;
		case 2:
			isMode2 = 1;
			break;
		case 3:
			signal(SIGINT, SIG_IGN);
			break;
		case 4:
			struct sigaction ctrlC;
			ctrlC.sa_sigaction = ctrl_c_handler;
			sigemptyset(&ctrlC.sa_mask);
			ctrlC.sa_flags = SA_SIGINFO;

			sigaction(SIGINT, &ctrlC, NULL);
			break;
		case 5:
			kill(info->si_pid, SIGUSR1);
			exit(0);
			break;
		default:
			printf("Wrong number. Try 1, 2, 3, 4 or 5\n\n");
			break;
	}
	kill(info->si_pid, SIGUSR1);
}

int main(){

	int mode2Counter = 0;

  	pid_t pid = getpid();
	printf("Catcher PID: %d\n\n", pid);

	struct sigaction action;
	action.sa_sigaction = sig_handle;
	sigemptyset(&action.sa_mask);
	action.sa_flags = SA_SIGINFO;

	sigaction(SIGUSR1, &action, NULL);

	while (1) {
		if (isMode2 == 1) {
			printf("%d\n", ++mode2Counter);
			sleep(1);
		}
		else {
			pause();
		}
	}

}