#include "lab8.h"

shared_print_queue *shared_queue_ptr = NULL;
int shm_fd = -1;
sem_t *mutex_sem = SEM_FAILED;
sem_t *empty_slots_sem = SEM_FAILED;
sem_t *filled_slots_sem = SEM_FAILED;



void initialize_shared_resources() {
    int created_shm = 0;

    shm_fd = shm_open(SHM_NAME, O_CREAT | O_EXCL | O_RDWR, 0666);
    if (shm_fd != -1) {
        printf("Created shared memory %s.\n", SHM_NAME);
        created_shm = 1;
        if (ftruncate(shm_fd, sizeof(shared_print_queue)) == -1) {
            perror("ftruncate failed after creating SHM");
            close(shm_fd);
            shm_unlink(SHM_NAME);
            exit(EXIT_FAILURE);
        }
    } else if (errno == EEXIST) {
        shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
        if (shm_fd == -1) {
            perror("shm_open (existing) failed");
            exit(EXIT_FAILURE);
        }
    } else {
        perror("shm_open (initial attempt) failed");
        exit(EXIT_FAILURE);
    }

    shared_queue_ptr = (shared_print_queue *)mmap(NULL, sizeof(shared_print_queue), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_queue_ptr == MAP_FAILED) {
        perror("mmap failed");
        close(shm_fd);
        if (created_shm) shm_unlink(SHM_NAME);
        exit(EXIT_FAILURE);
    }

    if (created_shm) {
        printf("Initializing shared memory data...\n");
        shared_queue_ptr->head = 0;
        shared_queue_ptr->tail = 0;
        shared_queue_ptr->count = 0;
        shared_queue_ptr->shutdown_flag = 0;
        memset(shared_queue_ptr->jobs, 0, sizeof(shared_queue_ptr->jobs));
    }

    mutex_sem = sem_open(SEM_MUTEX_NAME, O_CREAT | O_EXCL, 0666, 1);
    if (errno == EEXIST) {
        mutex_sem = sem_open(SEM_MUTEX_NAME, 0);
        if (mutex_sem == SEM_FAILED) {
          perror("sem_open (mutex existing) failed");
        }
    }
    else if (mutex_sem == SEM_FAILED) {
        perror("sem_open (mutex initial) failed");
    }

    empty_slots_sem = sem_open(SEM_EMPTY_SLOTS_NAME, O_CREAT | O_EXCL, 0666, QUEUE_CAPACITY);
    if (errno == EEXIST) {
        empty_slots_sem = sem_open(SEM_EMPTY_SLOTS_NAME, 0);
        if (empty_slots_sem == SEM_FAILED) {
          perror("sem_open (empty_slots existing) failed");
        }
    } else if (empty_slots_sem == SEM_FAILED) {
        perror("sem_open (empty_slots initial) failed");
    }

    filled_slots_sem = sem_open(SEM_FILLED_SLOTS_NAME, O_CREAT | O_EXCL, 0666, 0);
     if (errno == EEXIST) {
        filled_slots_sem = sem_open(SEM_FILLED_SLOTS_NAME, 0);
        if (filled_slots_sem == SEM_FAILED) {
          perror("sem_open (filled_slots existing) failed");
        }
     } else if (filled_slots_sem == SEM_FAILED) {
         perror("sem_open (filled_slots initial) failed");
     }

    printf("Successfully connected to all shared resources.\n");
    return;
}

void cleanup_local_resources() {
    printf("Cleaning up local resources...\n");
    if (shared_queue_ptr != NULL && shared_queue_ptr != MAP_FAILED) {
        munmap(shared_queue_ptr, sizeof(shared_print_queue));
        shared_queue_ptr = NULL;
    }
    if (shm_fd != -1) {
        close(shm_fd);
        shm_fd = -1;
    }
    if (mutex_sem != SEM_FAILED) {
        sem_close(mutex_sem);
        mutex_sem = SEM_FAILED;
    }
    if (empty_slots_sem != SEM_FAILED) {
        sem_close(empty_slots_sem);
        empty_slots_sem = SEM_FAILED;
    }
    if (filled_slots_sem != SEM_FAILED) {
        sem_close(filled_slots_sem);
        filled_slots_sem = SEM_FAILED;
    }
}