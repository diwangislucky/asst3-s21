#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <error.h>
#include <limits.h>
#include <pthread.h>

int fib_ser(int n)
{
  if (n < 2)
    return n;
  else
    return fib_ser(n-1) + fib_ser(n-2);
}

int fib(int n)
{
  int i, j;

  if (n <= 20)
    return fib_ser(n);
  else
  {
    #pragma omp task shared(i)
    i = fib(n-1);
    #pragma omp task shared(j)
    j = fib(n-2);
    #pragma omp taskwait
    return i+j;
  }
}

int main() {
  omp_set_num_threads(1);
  struct timespec before, after;
  clock_gettime(CLOCK_REALTIME, &before);
  int fib_res = fib(30);
  clock_gettime(CLOCK_REALTIME, &after);
  double delta_ms = (double)(after.tv_sec - before.tv_sec) * 1000.0 + (after.tv_nsec - before.tv_nsec) / 1000000.0;
  putchar('\n');
  printf("============ Time ============\n");
  printf("Time: %.3f ms (%.3f s)\n", delta_ms, delta_ms / 1000.0);
  printf("fib_res: %d\n", fib_res);
  return 0;
}
