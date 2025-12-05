#include <stdio.h>

#include "./MemoryManagment/mm.h"

int main(int argc, char *argv[]) {
  void *random_pointer = memory_alloc(sizeof(void *));
  printf("Random pointer adress: %p", &random_pointer);

  return 0;
}
