#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

void sig_handle(int signum) {
	printf("Handeling singal %d\n", signum);
}

int main(int argc, char *argv[]){

	if (argc != 2) {
		printf("Wrong number of arguments\nTry using: %s <what to do with SIG>\n", argv[0]);
		return 1;
	}

    if (strcmp(argv[1],"none") == 0){
		signal(SIGUSR1, SIG_DFL);
    }
	else if (strcmp(argv[1],"ignore") == 0) {
		signal(SIGUSR1, SIG_IGN);
	}
	else if (strcmp(argv[1],"handler") == 0) {
		signal(SIGUSR1, sig_handle);
	}
	else if (strcmp(argv[1],"mask") == 0) {
		sigset_t newmask;
		sigemptyset(&newmask);
		sigaddset(&newmask, SIGUSR1);
		if (sigprocmask(SIG_BLOCK, &newmask, NULL) < 0)
			perror("Nie udało się zablokować sygnału");
	}
	else {
		printf("Nie poprawna opcja: %s\n", argv[1]);
	}

	raise(SIGUSR1);

	if (strcmp(argv[1],"mask") == 0) {
		sigset_t pending_set;
		sigpending(&pending_set);

		if (sigismember(&pending_set, SIGUSR1)) {
			printf("SIGUSR1 jest oczekujący.\n");
		} else {
			printf("SIGUSR1 NIE oczekuje.\n");
		}
	}

	return 0;
}