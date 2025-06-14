#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

double fn(double x) {
	return 4.0 / ((x * x) + 1);
}

int main(int argc, char *argv[]) {

	if (argc != 3) {
		printf("Wrong number of arguments\n");
	}

	double start = atof(argv[1]);
	double end = atof(argv[2]);
    double segement = (end - start)/1000000.0;
	double sol = 0.0;

	FILE *pip = fopen("./pipe", "w");

	for (int j = 0; j < 1000000; j++) {
        double x = start + j*segement;
		sol += fn(x) * segement;
	}
	fwrite(&sol, sizeof(double), 1, pip);
	fclose(pip);

    return 0;
}