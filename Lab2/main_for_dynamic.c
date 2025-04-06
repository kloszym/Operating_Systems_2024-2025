#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>

void printing_func(int, int, int*, int);

int main(void){

  void *handle = dlopen("./lib/libcollatz_library.so", RTLD_LAZY);
  if(!handle){
	  printf("Couldn't open libcollatz_library.so\n");
  }

  int (*lib_fun)(int, int, int*);
  lib_fun = (int (*)(int, int, int*))dlsym(handle,"test_collatz_convergence");

  if(dlerror() != NULL){
    printf("Error: %s\n", dlerror());
    return 1;
  }

  int max_iter = 10;
  int input = 5;
  int * steps1 = (int*)malloc(max_iter*sizeof(int));
  int result = (*lib_fun)(input, max_iter, steps1);
  printing_func(input, max_iter, steps1, result);
  free(steps1);

  max_iter = 5;
  input = 9;
  int *steps2 = (int*)malloc(max_iter*sizeof(int));
  result = (*lib_fun)(input, max_iter, steps2);
  printing_func(input, max_iter, steps2, result);
  free(steps2);

  max_iter = 100;
  input = 1;
  int *steps3 = (int*)malloc(max_iter*sizeof(int));
  result = (*lib_fun)(input, max_iter, steps3);
  printing_func(input, max_iter, steps3, result);
  free(steps3);

  max_iter = 12;
  input = 159;
  int *steps4 = (int*)malloc(max_iter*sizeof(int));
  result = (*lib_fun)(input, max_iter, steps4);
  printing_func(input, max_iter, steps4, result);
  free(steps4);

  max_iter = 21;
  input = 7;
  int *steps5 = (int*)malloc(max_iter*sizeof(int));
  result = (*lib_fun)(input, max_iter, steps5);
  printing_func(input, max_iter, steps5, result);
  free(steps5);

  max_iter = 5;
  input = 1410;
  int *steps6 = (int*)malloc(max_iter*sizeof(int));
  result = (*lib_fun)(input, max_iter, steps6);
  printing_func(input, max_iter, steps6, result);
  free(steps6);

  dlclose(handle);

  return 0;
}

void printing_func(int input, int max_iter, int* steps, int result){

  printf("\n\nFor input: %d, max_iter: %d\n", input, max_iter);
  printf("Result: %d\n", result);
  for (int i=0; i<result; i++){
    printf("Step %d: %d\n", i, steps[i]);
  }
  if (input!=1 && result==0) {
    printf("In given max iteration it is imposiible to get to 1!!!\n");
  }
}