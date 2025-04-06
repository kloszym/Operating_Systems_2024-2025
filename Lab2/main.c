#include "collatz_library.h"
#include <stdlib.h>
#include <stdio.h>

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

void prepare_var(int oinput, int omax_iter){

  int max_iter = omax_iter;
  int input = oinput;
  int * steps = (int*)malloc(max_iter*sizeof(int));
  int result = test_collatz_convergence(input, max_iter, steps);
  printing_func(input, max_iter, steps, result);
  free(steps);

}

int main(void){

  prepare_var(5, 10);
  prepare_var(9, 5);
  prepare_var(1, 100);
  prepare_var(159, 12);
  prepare_var(7, 21);
  prepare_var(1410, 5);

  return 0;
}