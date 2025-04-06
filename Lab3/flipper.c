#define _GNU_SOURCE
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int control_sum(char* path) {
	FILE *file = fopen(path,"r");
	if (!file) {
		perror("Error opening source file");
		return -1;
	}
	int size = 0;
	while (fgetc(file) != EOF) {
		size++;
	}
	fclose(file);
	return size;
}


int main(int argc, char *argv[]){

	if (argc != 3) {
		printf("Try using: %s <source_directory> <destination_directory>\n", argv[0]);
		return 0;
	}

	DIR* directory = opendir(argv[1]);
	if (directory == NULL) {
		perror("Couldn't open source directory");
		return 0;
	}

	DIR* directory_result = opendir(argv[2]);
	if(directory_result == NULL) {
		if (mkdir(argv[2],S_IRWXU | S_IRWXG | S_IRWXO) != 0) {
			perror("mkdir couldn't create a directory");
			closedir(directory);
			return 0;
		}
		else {
			directory_result = opendir(argv[2]);
		}
	}


	struct dirent* entry;
	while((entry = readdir(directory)) != NULL) {
		char *filename = entry->d_name;
		char *extension = strrchr(filename, '.');
		if (strcmp(extension,".txt")==0) {

			char path_file[4096];
			char dest_file[4096];

			snprintf(path_file, sizeof(path_file), "%s/%s", argv[1], entry->d_name);
			snprintf(dest_file, sizeof(dest_file), "%s/%s", argv[2], entry->d_name);

			int original_size = control_sum(path_file);

			FILE *original_file = fopen(path_file,"r");
			if (!original_file) {
				perror("Error opening source file");
				continue;
			}

			FILE *result_file = fopen(dest_file,"w");
			if (!result_file) {
				perror("Error creating result file");
				continue;
			}

			FILE *count_file = fopen(path_file, "r");
			int total_lines = 0;
			char *temp_line = NULL;
			size_t temp_len = 0;
			while (getline(&temp_line, &temp_len, count_file) != -1) {
				total_lines++;
			}
			free(temp_line);
			fclose(count_file);

			rewind(original_file);

			char *line = NULL;
			size_t len = 0;
			ssize_t read;

			int line_count = 0;
			while ((read = getline(&line, &len, original_file)) != -1) {
				int len_line = strlen(line);
				int i, j;
				for (i = 0, j = len_line - 1; i < j; i++, j--) {
					char temp = line[i];
					line[i] = line[j];
					line[j] = temp;
				}
				line_count++;
				char *new_line = strdup(line);
				while (*new_line == '\n') {
					new_line++;
				}
				if (line_count != total_lines) {
					strcat(new_line, "\n");
				}


				if (fputs(new_line, result_file) == EOF) {
					perror("Error writing to output file");
					free(new_line);
					free(line);
					break;
				}
			}
			free(line);
			fclose(original_file);
			fclose(result_file);
			int result_size = control_sum(dest_file);

			printf("Size of '%s' is %d\nSize of '%s' is %d\n\n", path_file, original_size, dest_file, result_size);
		}
	}

	if (closedir(directory) < 0) {
		printf("Error closing original directory\n");
	}
	if (closedir(directory_result) < 0) {
		printf("Error closing result directory\n");
	}
	return 1;
}