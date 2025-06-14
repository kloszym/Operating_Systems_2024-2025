#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>

double fn(double x) {
	return 4.0 / ((x * x) + 1);
}

int main(int argc, char *argv[]) {
	if (argc != 3) {
		printf("Wrong number of arguments\n");
	}

	struct timeval start, end;

	double width_of_segment = atof(argv[1]);
	double how_many_segments = 1.0 / width_of_segment;
	int max_child_processes = atoi(argv[2]);

	int (*fds)[2] = malloc(sizeof(int[2]) * max_child_processes);
	int (*pids) = malloc(sizeof(int) * max_child_processes);

	for (int i = 1; i <= max_child_processes; i++) {
		gettimeofday(&start, NULL);
		int math_on_child = ceil(how_many_segments / i);
		for (int j = 0; j < i; j++) {
			double starting_point = j * math_on_child * width_of_segment;
			pipe(fds[j]);
			pid_t pid = fork();
			if (pid == 0) {
				close(fds[j][0]);
				double sum = 0;
				for (int k = 0; k < math_on_child; k++) {
					if (starting_point >= 1.0) {
						break;
					}

					sum += fn(starting_point) * width_of_segment;
					starting_point += width_of_segment;
				}

				write(fds[j][1], &sum, sizeof(double));
				close(fds[j][1]);
				exit(0);
			}
			else {
				pids[j] = pid;
				close(fds[j][1]);
			}
		}
		double to_read;
		double sol = 0.0;
		for (int j = 0; j < i; j++) {
			waitpid(pids[j], NULL, 0);
			read(fds[j][0], &to_read, sizeof(double));
			sol += to_read;
			close(fds[j][0]);
		}

		gettimeofday(&end, NULL);
		double diff_t = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
		printf("k: %d, sol: %f, time: %f\n", i, sol, diff_t);
	}
	free(fds);
	free(pids);
	return 0;
}
