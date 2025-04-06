#include "collatz_library.h"

int collatz_conjecture(int input){

  return input%2==0 ? input/2 : (3*input)+1;

}
int test_collatz_convergence(int input, int max_iter, int *steps){

  if (input == 1) return 0;
  steps[0] = collatz_conjecture(input);
  int i = 1;
  while (i < max_iter && steps[i-1]!=1) {
    steps[i] = collatz_conjecture(steps[i-1]);
    i++;
  }

  return steps[max_iter-1]!=1 && i==max_iter ? 0 : i;

}