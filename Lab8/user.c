#include "lab8.h"

int user_keep_running = 1;

void user_signal_handler(int sig) {
    user_keep_running = 0;
    if (shared_queue_ptr != NULL && shared_queue_ptr != MAP_FAILED) {
        shared_queue_ptr->shutdown_flag = 1;
    }
}

void generate_random_string(char *str, size_t size) {
    const char chars[] = "abcdefghijklmnopqrstuvwxyz";
    if (size) {
        --size;
        for (size_t n = 0; n < size; n++) {
            int key = rand() % (int)(sizeof(chars) - 1);
            str[n] = chars[key];
        }
        str[size] = '\0';
    }
}

int main() {

    signal(SIGINT, user_signal_handler);
    signal(SIGTERM, user_signal_handler);

    srand(time(NULL));
    initialize_shared_resources();

    while (user_keep_running && (shared_queue_ptr == NULL || shared_queue_ptr == MAP_FAILED || !shared_queue_ptr->shutdown_flag)) {
        print_job_t new_job;
        generate_random_string(new_job.text, WORD_SIZE);

        if (sem_wait(empty_slots_sem) == -1) {
            if (errno == EINTR && !user_keep_running) break;
            if (errno == EINTR) continue;
            printf("User: sem_wait (empty_slots)"); break;
        }
        if (!user_keep_running || (shared_queue_ptr != NULL && shared_queue_ptr != MAP_FAILED && shared_queue_ptr->shutdown_flag)) {
            sem_post(empty_slots_sem);
            break;
        }

        if (sem_wait(mutex_sem) == -1) {
            if (errno == EINTR && !user_keep_running) {
                sem_post(empty_slots_sem);
                break;
            }
            if (errno == EINTR) {
                sem_post(empty_slots_sem);
                continue;
            }
            perror("User: sem_wait (mutex)");
            sem_post(empty_slots_sem);
            break;
        }
        if (!user_keep_running || (shared_queue_ptr != NULL && shared_queue_ptr != MAP_FAILED && shared_queue_ptr->shutdown_flag)) {
            sem_post(mutex_sem);
            sem_post(empty_slots_sem);
            break;
        }

        if (shared_queue_ptr != NULL && shared_queue_ptr != MAP_FAILED) {
            shared_queue_ptr->jobs[shared_queue_ptr->tail] = new_job;
            shared_queue_ptr->tail = (shared_queue_ptr->tail + 1) % QUEUE_CAPACITY;
            shared_queue_ptr->count++;
            printf("Enqueued job '%s'. Queue: %d/%d\n", new_job.text, shared_queue_ptr->count, QUEUE_CAPACITY);
        }

        sem_post(mutex_sem);
        sem_post(filled_slots_sem);

        int sleep_time = (rand() % 10) + 1;
        sleep(sleep_time);
    }

    cleanup_local_resources();
    printf("Exited.\n");
    return 0;
}