#ifndef MM2_H
#define MM2_H

#include <stddef.h>
#include <stdint.h>

// so now we want to make a memory allocator which gets a big chunck at the
// beginning of the programm and then splits it so this means we still need a
// header struct but some things are going to be different now
typedef struct Block_Header {
  size_t size;
  unsigned free;
  struct Block_Header *next;
} block_header __attribute__((aligned(16)));
// its basically the same struct we just change the name else nothing is
// different

// we now need a function to init the chunck we have will use later
void init_block();

// then we also have to make our own malloc function
void *memory_from_block(size_t size);

// and we also have to have a function which at the end of the programm releases
// everything
void release_memory();

#endif
