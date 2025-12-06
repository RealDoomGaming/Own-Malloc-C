#include <stdio.h>
#include <unistd.h>

#include "./MemoryManagment/UsingChunck/mm.h"
#include "./MemoryManagment/UsingSbrk/mm.h"

int main(int argc, char *argv[]) {
  void *random_pointer = memory_alloc(sizeof(void *));
  printf("Random pointer adress: %p\n", &random_pointer);

  void *programmbreak = sbrk(0);
  printf("Programm break adress: %p\n", &programmbreak);

  return 0;
}
