#include <bits/time.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "./MemoryManagment/UsingChunck/mm.h"

void benchmark_small_allocs(int which);
double get_time_ms();

int main(int argc, char *argv[]) {
  // now since I wanted to test this with a benchmark I will do so
  printf("==========================================\n");
  printf("Small Allocation Benchmark (64 bytes)\n");
  printf("10.000 Allocations + 10.000 Frees\n");
  printf("==========================================\n\n");

  // 0 stand sfor the standart malloc
  benchmark_small_allocs(0);
  // and 1 stands for my own malloc
  benchmark_small_allocs(1);

  return 0;
}

void benchmark_small_allocs(int which) {
  const int NUM_ALLOCS = 10000;
  const size_t SIZE = 64;

  void **pointers = malloc(NUM_ALLOCS * sizeof(void *));

  double start = get_time_ms();
  for (int i = 0; i < NUM_ALLOCS; i++) {
    if (which) {
      pointers[i] = memory_from_block(SIZE);
    } else {
      pointers[i] = malloc(SIZE);
    }
  }
  double alloc_time = get_time_ms() - start;

  start = get_time_ms();
  for (int i = 0; i < NUM_ALLOCS; i++) {
    if (which) {
      free_memory(pointers[i]);
    } else {
      free(pointers[i]);
    }
  }
  double free_time = get_time_ms() - start;

  printf("%s malloc\n", which ? "Custome" : "Standard");
  printf("Allocations: %.3f ms\n", alloc_time);
  printf("Frees: %.3f ms\n", free_time);
  printf("Total: %.3f ms\n\n", alloc_time + free_time);
}

double get_time_ms() {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec * 1000.0 + ts.tv_nsec / 1000000.0;
}
