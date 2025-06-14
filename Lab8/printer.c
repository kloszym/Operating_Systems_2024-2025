#include "lab8.h"

int printer_keep_running = 1;
int printer_id_num = -1;


void printer_signal_handler(int sig) {
    printer_keep_running = 0;
    if (shared_queue_ptr != NULL && shared_queue_ptr != MAP_FAILED) {
        shared_queue_ptr->shutdown_flag = 1;
    }
}

int main() {

    signal(SIGINT, printer_signal_handler);
    signal(SIGTERM, printer_signal_handler);

    initialize_shared_resources();

    printf("Waiting for jobs...\n");

    while (printer_keep_running && (shared_queue_ptr == NULL || shared_queue_ptr == MAP_FAILED || !shared_queue_ptr->shutdown_flag)) {
        print_job_t current_job;

        if (sem_wait(filled_slots_sem) == -1) {
            if (errno == EINTR && !printer_keep_running) break;
            if (errno == EINTR) continue;
            perror("Printer: sem_wait (filled_slots)");
            break;
        }
        if (!printer_keep_running || (shared_queue_ptr != NULL && shared_queue_ptr != MAP_FAILED && shared_queue_ptr->shutdown_flag)) {
            sem_post(filled_slots_sem);
            break;
        }

        if (sem_wait(mutex_sem) == -1) {
            if (errno == EINTR && !printer_keep_running) {
                sem_post(filled_slots_sem);
                break;
            }
            if (errno == EINTR) {
                sem_post(filled_slots_sem);
                continue;
            }
            perror("Printer: sem_wait (mutex)");
            sem_post(filled_slots_sem);
            break;
        }
         if (!printer_keep_running || (shared_queue_ptr != NULL && shared_queue_ptr != MAP_FAILED && shared_queue_ptr->shutdown_flag)) {
             sem_post(mutex_sem);
             sem_post(filled_slots_sem);
             break;
        }

        if (shared_queue_ptr != NULL && shared_queue_ptr != MAP_FAILED) {
            current_job = shared_queue_ptr->jobs[shared_queue_ptr->head];
            shared_queue_ptr->head = (shared_queue_ptr->head + 1) % QUEUE_CAPACITY;
            shared_queue_ptr->count--;
            printf("Dequeued job '%s' from User. Queue: %d/%d. Printing...\n", current_job.text, shared_queue_ptr->count, QUEUE_CAPACITY);
        } else {
            sem_post(mutex_sem);
            sem_post(empty_slots_sem);
            continue;
        }

        sem_post(mutex_sem);
        sem_post(empty_slots_sem);

        for (int i = 0; current_job.text[i] != '\0'; ++i) {
            if(!printer_keep_running) break;
            printf("%c\n", current_job.text[i]);
            fflush(stdout);
            sleep(1);
        }
        if(printer_keep_running && current_job.text[0] != '\0') printf("Finished printing job '%s'.\n", current_job.text);
    }

    cleanup_local_resources();
    printf("Exited.\n");
    return 0;
}