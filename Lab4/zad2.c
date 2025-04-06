#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

int global = 0;

int main(int argc, char *argv[]){

	if (argc != 2) {
		printf("Wrong number of arguments\nTry using: %s <directory>\n", argv[0]);
		return 1;
	}

	pid_t child_pid = 1;
    int local = 0;
    printf("Program glowny zad2\n");
	child_pid = fork();
	if(child_pid!=0) {
    	int status;
        waitpid(child_pid, &status, 0);
		printf("parent process\n");
		printf("parent pid = %d, child pid = %d\n", (int)getpid(), (int)child_pid);
		printf("Child exit code: %d\n", WEXITSTATUS(status));
		printf("Parent's local = %d, parent's global = %d\n", local, global);
        if (WIFEXITED(status)) {
        	return WEXITSTATUS(status);
    	} else {
        	return 4;
    	}
	} else {
		printf("child process\n");
        global++;
        local++;
		printf("child pid = %d, parent pid = %d\n", (int)getpid(), (int)getppid());
		printf("child's local = %d, child's global = %d\n", local, global);
        int res = execl("/bin/ls", "ls", argv[1], NULL);
        if (res == -1) {
            perror("execl error");
            exit(3);
        }
	}
	return 0;
}