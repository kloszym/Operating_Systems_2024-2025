#ifndef LAB8_H
#define LAB8_H

#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <time.h>

#define SHM_NAME "/shm"
#define SEM_MUTEX_NAME "/mutex"
#define SEM_EMPTY_SLOTS_NAME "/empty"
#define SEM_FILLED_SLOTS_NAME "/filled"

#define WORD_SIZE 10
#define QUEUE_CAPACITY 10

typedef struct {
	char text[WORD_SIZE + 1];
} print_job_t;

typedef struct {
	print_job_t jobs[QUEUE_CAPACITY];
	int head;
	int tail;
	int count;
	volatile sig_atomic_t shutdown_flag;
} shared_print_queue;

extern shared_print_queue *shared_queue_ptr;
extern int shm_fd;
extern sem_t *mutex_sem, *empty_slots_sem, *filled_slots_sem;

void initialize_shared_resources();
void cleanup_local_resources();

#endif //LAB8_H