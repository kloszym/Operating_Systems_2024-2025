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
#include <time.h>
#include <stdbool.h>
#include <locale.h>

#define MEDICINE_BOX_CAPACITY 6

typedef struct{

	int id;
	bool is_cured;
	pthread_cond_t is_cured_cond;
	pthread_t patient_thread;

}patient_info;

typedef struct {

	int id;
	pthread_t pharmacist_thread;

} pharmacist_info;

size_t amount_of_patients;
size_t amount_of_pharmacists;

size_t clinic = 0;
patient_info* patients_in_clinic[3];
pthread_mutex_t clinic_mutex = PTHREAD_MUTEX_INITIALIZER;

size_t medicine_box = MEDICINE_BOX_CAPACITY;
pthread_mutex_t medicine_box_mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t is_clinic_full = PTHREAD_COND_INITIALIZER;
pthread_cond_t is_medicine_box_near_empty = PTHREAD_COND_INITIALIZER;

patient_info* patients;
pharmacist_info* pharmacists;

char* current_time() {
	static char bufor[64];
	time_t czas;
	time( & czas );
	struct tm czasTM = * localtime( & czas );

	setlocale( LC_ALL, "Polish" );
	strftime( bufor, sizeof( bufor ), "%H:%M:%S", & czasTM );
	return bufor;
}

void* doctor(void* ptr) {

	while (amount_of_patients > 0) {
		if (pthread_mutex_lock(&medicine_box_mutex) != 0) {
			printf("%s - Lekarz: Mutex lock failed\n", current_time());
		}
		while (medicine_box < 3 && amount_of_patients > 0) {
			pthread_cond_wait(&is_medicine_box_near_empty, &medicine_box_mutex);
			printf("\033[34m%s - Lekarz: przyjmuję dostawę leków\n", current_time());
			medicine_box = MEDICINE_BOX_CAPACITY;
			int wait_time = (rand() % 3) + 1;
			printf("%s - Lekarz: Zasypiam\n", current_time());
			sleep(wait_time);
		}
		if (pthread_mutex_unlock(&medicine_box_mutex) != 0) {
			printf("%s - Lekarz: Mutex unlock failed\n", current_time());
		}
		if (pthread_mutex_lock(&clinic_mutex) != 0) {
			printf("%s - Lekarz: Mutex lock failed\n", current_time());
		}
		while (clinic < 3 && amount_of_patients > 0) {
			pthread_cond_wait(&is_clinic_full, &clinic_mutex);
		}

		if (amount_of_patients == 0) {
			pthread_mutex_unlock(&clinic_mutex);
			break;
		}

		if (clinic == 3 && amount_of_patients >= 3) {
			printf("\033[34m%s - Lekarz: konsultuję pacjentów %d, %d, %d\n", current_time(), patients_in_clinic[0]->id, patients_in_clinic[1]->id, patients_in_clinic[2]->id);

			int wait_time = (rand() % 3) + 2;
			sleep(wait_time);
			if (pthread_mutex_lock(&medicine_box_mutex) != 0) {
				printf("%s - Lekarz: Mutex lock failed\n", current_time());
			}

			for (int k = 0; k < 3; k++) {
				medicine_box--;
				patients_in_clinic[k]->is_cured = true;
				pthread_cond_signal(&patients_in_clinic[k]->is_cured_cond);
			}

			if (pthread_mutex_unlock(&medicine_box_mutex) != 0) {
				printf("%s - Lekarz: Mutex unlock failed\n", current_time());
			}

			amount_of_patients-=3;
			clinic = 0;

			if (pthread_mutex_unlock(&clinic_mutex) != 0) {
				printf("%s - Lekarz: Mutex unlock failed\n", current_time());
			}
			printf("\033[34m%s - Lekarz: Zasypiam\n", current_time());
		}
		else {
			if (pthread_mutex_unlock(&clinic_mutex) != 0) {
				printf("%s - Lekarz: Mutex unlock failed\n", current_time());
			}
			if (amount_of_patients > 0 && amount_of_patients < 3) {
				amount_of_patients = 0;
			}
		}

	}
	printf("\033[34m%s - Lekarz: Koniec na dzisiaj\n", current_time());
	pthread_exit(0);

}

void* patient(void* ptr) {
	patient_info* info = (patient_info*)ptr;
	int id = info->id;

	int wait_time = (rand() % 4) + 2;
	printf("\033[33m%s - Pacjent%d: Ide do szpitala, bede za %d s\n", current_time(), id, wait_time);
	sleep(wait_time);
	bool is_in_clinic = false;
	while (!is_in_clinic) {
		if (pthread_mutex_lock(&clinic_mutex) != 0) {
			printf("%s - Pacjent%d: Mutex lock failed\n", current_time(), id);
		}
		if (clinic < 3){
			is_in_clinic = true;
			patients_in_clinic[clinic] = info;
			clinic++;
			printf("\033[33m%s - Pacjent%d: czeka %lu pacjentów na lekarza\n", current_time(), id, clinic);
			if (clinic == 3) {

				printf("\033[33m%s - Pacjent%d: budzę lekarza\n", current_time(), id);
				pthread_cond_broadcast(&is_clinic_full);

			}
			if (pthread_mutex_unlock(&clinic_mutex) != 0) {
				printf("%s - Pacjent%d: Mutex unlock failed\n", current_time(), id);
			}

		}
		else{
			wait_time = (rand() % 4) + 2;
			printf("\033[33m%s - Pacjent%d: za dużo pacjentów, wracam później za %d s\n", current_time(), id, wait_time);
			if (pthread_mutex_unlock(&clinic_mutex) != 0) {
				printf("%s - Pacjent%d: Mutex unlock failed\n", current_time(), id);
			}
			sleep(wait_time);

		}

	}
	if (pthread_mutex_lock(&clinic_mutex) != 0) {
		printf("%s - Pacjent%d: Mutex lock failed\n", current_time(), id);
	}
	while (!info->is_cured) {
		pthread_cond_wait(&info->is_cured_cond, &clinic_mutex);
	}
	printf("\033[33m%s - Pacjent%d: kończę wizytę\n", current_time(), id);
	if (pthread_mutex_unlock(&clinic_mutex) != 0) {
		printf("%s - Pacjent%d: Mutex unlock failed\n", current_time(), id);
	}
	pthread_exit(0);
}

void* pharmacist(void* ptr) {
	pharmacist_info* info = (pharmacist_info*)ptr;
	int id = info->id;

	bool is_delivered = false;

	int wait_time = (rand() % 11) + 5;
	printf("\033[31m%s - Farmaceuta%d: Ide do szpitala, bede za %d s\n", current_time(), id, wait_time);
	sleep(wait_time);

	while (!is_delivered) {

		if (pthread_mutex_lock(&medicine_box_mutex) != 0){
			printf("%s - Farmaceuta%d: Mutex lock failed\n", current_time(), id);
		}
		if (medicine_box < 3){
			printf("\033[31m%s - Farmaceuta%d: budzę lekarza\n", current_time(), id);
			pthread_cond_signal(&is_medicine_box_near_empty);
			printf("\033[31m%s - Farmaceuta%d: dostarczam leki\n", current_time(), id);
			is_delivered = true;
		}
		if (pthread_mutex_unlock(&medicine_box_mutex) != 0) {
			printf("%s - Farmaceuta%d: Mutex unlock failed\n", current_time(), id);
		}
		if (!is_delivered) {
			sleep(1);
		}
	}

	printf("\033[31m%s - Farmaceuta%d: zakończyłem dostawę\n", current_time(), id);
	pthread_exit(0);
}

int main(int argc,char* argv[]){

	if (argc != 3) {
		printf("Usage: %s {patients} {pharmacists}\n", argv[0]);
		exit(-1);
	}

	srand(time(NULL));
	amount_of_patients = atoi(argv[1]);
	amount_of_pharmacists = atoi(argv[2]);

	patients = (patient_info*)malloc(amount_of_patients*sizeof(patient_info));
	pharmacists = (pharmacist_info*)malloc(amount_of_pharmacists*sizeof(pharmacist_info));

	pthread_t doctor_thread;
	if (pthread_create(&doctor_thread, NULL, doctor, NULL)!=0) {
		printf("\033[0mError creating wątek Lekarza\n");
		exit(-1);
	}

	size_t max = amount_of_patients > amount_of_pharmacists ? amount_of_patients : amount_of_pharmacists;
	for (int i = 0; i < max; i++) {
		if (i < amount_of_patients) {
			patient_info* info = &(patients[i]);
			info->id = i;
			info->is_cured = false;
			pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
			info->is_cured_cond = cond;
			pthread_create(&info->patient_thread, NULL, patient, info);
		}
		if (i<amount_of_pharmacists) {
			pharmacist_info* info = &(pharmacists[i]);
			info->id = i;
			pthread_create(&info->pharmacist_thread, NULL, pharmacist, info);
		}
	}
	pthread_join(doctor_thread, NULL);
	for (int i = 0; i < max; i++) {
		if (i < amount_of_patients) {
			pthread_join(patients[i].patient_thread, NULL);
		}
		if (i<amount_of_pharmacists) {
			pthread_join(pharmacists[i].pharmacist_thread, NULL);
		}
	}

	printf("\033[0m\n");

	free(patients);
	free(pharmacists);
}