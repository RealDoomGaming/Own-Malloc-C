#ifndef MM_H
#define MM_H

#include <stddef.h>
#include <stdint.h>

// this is a struct for our memory, size holds the current memory the struct has
// and which it occupies the free variable tells us if this struct (memory
// adress) is currently being occupied and the next is just the next availabe
// struct we have (like a linked list ) and the struct which came before
typedef struct Header_T {
  size_t size;
  unsigned free;
  struct Header_T *next;
} header_t __attribute__((aligned(16)));
// the aligned 16 is for efficency and else the program would automatically put
// it at 8 bytes because thats the biggest variable in our struct (the pointer)
// but we only need 16

// this function will just allocate memory with using sbrk for smaller work
// loads and mmap for bigger work loads It will basically just look in our
// linked list if we have an available header_t which has enough size
void *memory_alloc(size_t size);

// free_memory wil basically just be my own function of the free function
// already this just checks if the current block is at the end of the stack then
// we release memory to the os else we just mark it as free
void free_memory(void *block);

#endif
