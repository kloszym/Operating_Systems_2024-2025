#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char *argv[]){

  if (argc != 2) {
    printf("Wrong number of arguments\nTry using: %s <number>\n", argv[0]);
  }

  pid_t child_pid = 1;
  int number = atoi(argv[1]);
  for (int i = 0; i < number && child_pid!=0; i++) {
    child_pid = fork();
    if(child_pid!=0) {
      wait(NULL);
    } else {
      printf("PPID: %d; PID: %d\n",(int)getppid(), (int)getpid());
      }
    }
  if (child_pid != 0) {
    printf("%d\n", number);
  }
  return 0;
}