#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char *argv[]) {

	if (argc != 3) {
		printf("Wrong number of arguments\n");
	}

	double sol;

    if (mkfifo("./pipe", 0777) < 0){
    	perror("Error making named pipe");
    	exit(EXIT_FAILURE);
    }




	pid_t pid = fork();

	if(pid==0) {
		execl("./zad2cat", "zad2cat", argv[1], argv[2], NULL);
		perror("execl failed");
		exit(EXIT_FAILURE);
	}
    else {
    	FILE *pip = fopen("./pipe", "r");
    	if (pip == NULL) {
    		perror("fopen");
    		exit(EXIT_FAILURE);
    	}
		waitpid(pid, NULL, 0);
		fread(&sol, sizeof(double), 1, pip);
    	fclose(pip);
	}

	remove("./pipe");
	printf("result: %f\n", sol);
	return 0;

}