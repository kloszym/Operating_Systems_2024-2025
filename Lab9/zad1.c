#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>
#include <pthread.h>

double width_of_segment;
double how_many_segments;
int how_many_threads;
int math_on_thread;

double* results;
pthread_t* threads;

typedef struct {
	int id;
	double x;
} thread_data;

thread_data* datas;

double fn(double x) {
	return 4.0 / ((x * x) + 1);
}

void* calc_integral(void *ptr) {
	thread_data* data = (thread_data *) ptr;
	int index = data->id;
	double curr_x = data->x;
	double sum = 0;
	for (int i = 0; i < math_on_thread && curr_x < 1.0; i++) {
		sum += fn(curr_x) * width_of_segment;
		curr_x += width_of_segment;
	}
	results[index]=sum;
	return NULL;
}

int main(int argc, char *argv[]) {

	if (argc != 3) {
		printf("Try %s {width of segment} {how many threads}\n", argv[0]);
		exit(-1);
	}

	width_of_segment = atof(argv[1]);
	how_many_segments = 1.0 / width_of_segment;
	how_many_threads = atoi(argv[2]);
	math_on_thread = ceil(how_many_segments / how_many_threads);

	results = calloc(how_many_threads, sizeof(double));
	threads = malloc(sizeof(pthread_t) * how_many_threads);

	double curr_x = 0.0;
	datas = malloc(sizeof(thread_data) * how_many_threads);

	for (int i = 0; i < how_many_threads; i++) {

		datas[i].id = i;
		datas[i].x = curr_x;

		if (pthread_create(&(threads[i]), NULL, calc_integral, (void*)&(datas[i])) != 0) {
			perror("pthread_create");
			free(results);
			free(threads);
			free(datas);
			exit(-1);
		}
		curr_x += math_on_thread * width_of_segment;
	}

	for (int i = 0; i < how_many_threads; i++) {
		if (pthread_join(threads[i], NULL) != 0) {
			perror("pthread_join");
		}
	}

	double sol = 0.0;
	for (int i = 0; i < how_many_threads; i++) {
		sol+=results[i];
	}

	printf("Result: %lf\n", sol);

	free(results);
	free(threads);
	free(datas);
	return 0;
}
