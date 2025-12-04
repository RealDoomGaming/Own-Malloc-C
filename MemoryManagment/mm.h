#ifndef MM_H
#define MM_H

#include <stddef.h>
#include <stdint.h>

// this is a struct for our memory, size holds the current memory the struct has
// and which it occupies the free variable tells us if this struct (memory
// adress) is currently being occupied and the next is just the next availabe
// struct we have (like a linked list )
typedef struct Header_T {
  size_t sizeOfCurrent;
  unsigned free;
  struct Header_T *next;
} header_t __attribute__((aligned(16)));
// the aligned 16 is for efficency and else the program would automatically put
// it at 24 bytes but we only need 16

#endif
