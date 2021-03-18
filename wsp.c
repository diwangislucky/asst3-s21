
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <error.h>
#include <limits.h>
#include <pthread.h>

// TODO: You may find this helpful.
#include <omp.h>

/*
________________________________________________

 FRIENDLY ADVICE FROM YOUR TAs:
________________________________________________

0) Read over the file, familiarizing yourself
with the provided functions. You are free to
change structs and add additional helper 
functions, but should not modify the
argument parsing or printing mechanisms. 

1) Start by developing a sequential solution.
A parallelized approach is useless here if it 
sacrifices correctness for speed.

2) Run the verify.py script to verify the
correctness of your outputs compared to
our reference solution on smaller test cases.
We also encourage you to compare your results
against the reference manually on larger
test cases. 

NOTE: there may be multiple correct paths 
for the same distance file. We are only
looking for you to produce the correct
distance cost.

3) Start with simple parallelization
techniques that do not require large
restructuring of your code (Hint: openMP
may come in handy at this stage). Be mindful
of not accidentally introducing uninetntional
dependencies within parallel sections of code
that will unwantingly serialize code.

4) If you still do not have enough of a 
performance increase, you may need to 
introduce larger structural changes to
your code to help make it more amendable
to parallelism. Attempt small iterative
changes while still ensuring the correctness
of your code.
________________________________________________
*/ 

#define SYSEXPECT(expr) do { if(!(expr)) { perror(__func__); exit(1); } } while(0)
#define error_exit(fmt, ...) do { fprintf(stderr, "%s error: " fmt, __func__, ##__VA_ARGS__); exit(1); } while(0);

typedef int8_t city_t;

int NCORES = -1;  // TODO: this isn't being used anywhere, this needs to be used somewhere.
int NCITIES = -1; // number of cities in the file.
int *DIST = NULL; // one dimensional array used as a matrix of size (NCITIES * NCITIES).


typedef struct path_struct_t {
  int cost;         // path cost.
  city_t *path;     // order of city visits (you may start from any city).
} path_t;
path_t *bestPath = NULL;

// set DIST[i,j] to value
// what's cache line size?
inline static void set_dist(int i, int j, int value) {
  assert(value > 0);
  int offset = i * NCITIES + j;
  DIST[offset] = value;
  return;
}

// returns value at DIST[i,j]
inline static int get_dist(int i, int j) {
  int offset = i * NCITIES + j;
  return DIST[offset];
}

// prints results
void wsp_print_result() {
  printf("========== Solution ==========\n");
  printf("Cost: %d\n", bestPath->cost);
  printf("Path: ");
  for(int i = 0; i < NCITIES; i++) {
    if(i == NCITIES-1) printf("%d", bestPath->path[i]);
    else printf("%d -> ", bestPath->path[i]);
  }
  putchar('\n');
  putchar('\n');
  return;
}

static inline void swap(city_t *ptr1, city_t *ptr2) {
    city_t temp = *ptr1;
    *ptr1 = *ptr2;
    *ptr2 = temp;
    return;
}

city_t *scratch;
// how can we make a threadlocal copy of scratch?
// without making it seem like a dick move
void wsp_print_scratch() {
  printf("Path: ");
  for(int i = 0; i < NCITIES; i++) {
    if(i == NCITIES-1) printf("%d", scratch[i]);
    else printf("%d -> ", scratch[i]);
  }
  putchar('\n');
  return;
}

// best path is initialized to idxs
// let's avoid using heuristics.
int wsp_recursion(int idx, int sum_dist) {
  if (idx == NCITIES) {
    wsp_print_scratch();
    return sum_dist;
  }
  int min_dist = INT32_MAX;
  for (int i = idx; i < NCITIES; ++i) {
    swap(scratch + idx, scratch + i);
    int rem_dist;
    if (idx > 0) {
      city_t pIdx1 = scratch[idx - 1];
      city_t pIdx2 = scratch[idx];
      rem_dist = wsp_recursion(idx + 1, sum_dist + get_dist(pIdx1, pIdx2));
    }
    else {
      rem_dist = wsp_recursion(idx + 1, sum_dist);
    }
    if (rem_dist < min_dist) {
      min_dist = rem_dist;
      if (idx == 0) {
	memcpy(bestPath->path, scratch, NCITIES * sizeof(city_t));
      }
    }
    swap(scratch + idx, scratch + i);
  }
  return min_dist;
}

// We can have the other threads just wait. Locking is costly`
void wsp_start() {
  int cityID = 0;
  for(cityID=0; cityID < NCITIES; cityID++) {
    scratch[cityID] = cityID;
  }

  printf("NCITIES: %d\n", NCITIES);
  int min_cost = wsp_recursion(0, 0);
  bestPath->cost = min_cost;

  return;
}

int main(int argc, char **argv) {
  if(argc < 4 || strcmp(argv[1], "-p") != 0) error_exit("Expecting two arguments: -p [processor count] and [file name]\n");
  NCORES = atoi(argv[2]);
  if(NCORES < 1) error_exit("Illegal core count: %d\n", NCORES);
  char *filename = argv[3];
  FILE *fp = fopen(filename, "r");
  if(fp == NULL) error_exit("Failed to open input file \"%s\"\n", filename);
  int scan_ret;
  scan_ret = fscanf(fp, "%d", &NCITIES);
  if(scan_ret != 1) error_exit("Failed to read city count\n");
  if(NCITIES < 2) {
    error_exit("Illegal city count: %d\n", NCITIES);
  } 
  // Allocate memory and read the matrix
  DIST = (int*)calloc(NCITIES * NCITIES, sizeof(int));
  SYSEXPECT(DIST != NULL);
  for(int i = 1;i < NCITIES;i++) {
    for(int j = 0;j < i;j++) {
      int t;
      scan_ret = fscanf(fp, "%d", &t);
      if(scan_ret != 1) error_exit("Failed to read dist(%d, %d)\n", i, j);
      set_dist(i, j, t);
      set_dist(j, i, t);
    }
  }
  fclose(fp);
  bestPath = (path_t*)malloc(sizeof(path_t));
  bestPath->cost = 0;
  bestPath->path = (city_t*)calloc(NCITIES, sizeof(city_t));
  scratch = (city_t*)calloc(NCITIES, sizeof(city_t));
  struct timespec before, after;
  clock_gettime(CLOCK_REALTIME, &before);
  wsp_start();
  clock_gettime(CLOCK_REALTIME, &after);
  double delta_ms = (double)(after.tv_sec - before.tv_sec) * 1000.0 + (after.tv_nsec - before.tv_nsec) / 1000000.0;
  putchar('\n');
  printf("============ Time ============\n");
  printf("Time: %.3f ms (%.3f s)\n", delta_ms, delta_ms / 1000.0);
  wsp_print_result();
  return 0;
}
