#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mm.h"

// at first we have to define what size we want to init our block with, this
// could be anything but I would go for 1 MB because we wouldnt really need
// anything bigger for my use cases you could also use a parameter with the
// init_block function yourself if you wanted to
#define INIT_SIZE (1024 * 1024)
#define BLOCK_SIZE sizeof(block_header)
// then we define a min alloc variable for later usage (you will see)
#define MIN_ALLOC_SIZE (16)

// we only need a list for all the free elements in the linked list
block_header *first_free = NULL, *last_free = NULL;
// we also make a heap start so we know where our own heap we will make starts
void *heap_start = NULL;

// this next variable is for tracking if we make an alloc for the first time
int first_time = 1;

pthread_mutex_t global_lock;

// our future function for funding the first header in the linked list which is
// free and has the right amount of size
block_header *find_first_free(size_t size);
// now we also need to make a function which splits the memory if its too large
// but not if the rest is only enough for a header
void split_block(block_header *header, size_t size);
// because of performance reasons and also because it could come in handy we
// could merge free blocks together when two or more which are next to eachother
// are free
void merge_block(block_header *header);

void init_block() {
  // first thing we do is give our heap start all our allocated memory
  heap_start = sbrk(INIT_SIZE);
  // then we want to check if this was successfull ot not
  if (heap_start == (void *)-1) {
    perror("Initing Block");
    printf("There was an error initing the block because the sbrk failed\n");
    return;
  }

  // else if our initial init was sucessfull we can continue with doing the
  // other stuff
  // we firstly make our first header which then has all the memory behind it as
  // its memory which it will give to the user
  block_header *header = (block_header *)heap_start;
  header->size = INIT_SIZE - BLOCK_SIZE;
  // we then also set the free to true (1) and set the next in the linked list
  // to NULL because this element is the  first element in the linked list
  header->free = 1;
  header->next_free = NULL;
  header->prev_free = NULL;
  first_free = header;
  last_free = header;
}
// with that our init block function is completed
// now we can move onto our actuall alloc function

void *memory_from_block(size_t size) {
  // in this function we basically just have to either find a free header which
  // has the right amount of memory we need if we find one which is free and has
  // to much memory for us we will take as much as we need and make the rest
  // into its own block as long as the rest memory is bigger then the header
  // size
  // else if we could find anything like that we make a new one and if we cant
  // make a new one we just have to request memory from the os

  // first thing we do is lock our mutex
  pthread_mutex_lock(&global_lock);

  // after that we do is check if this is the first time we make a new alloc so
  // then we can make the heap first
  if (first_time) {
    init_block();
    first_time = 0;
  }

  block_header *header = find_first_free(size);
  // we dont need to return here if we find one because if we init the block
  // first there will alaways be a header for us read // we dont need to return
  // here if we find one because if we init the block first there will alaways
  // be a header for us readyy

  // but if we couldnt find any header with the right size we know that we need
  // more memory from the os
  if (!header) {
    // we can get the new size we need from the os with sbrk and the size of the
    // block and the size the user wants
    header = sbrk(BLOCK_SIZE + size);

    // then we can do all the other stuff we need with the header
    header->size = size;
    header->free = 0;
    // since its not free we dont need to do any linked list stuff since our
    // linked list only includes the free elements
    header->next_free = NULL;
    header->prev_free = NULL;

    // before we return our new header we have to unlock the mutex
    pthread_mutex_unlock(&global_lock);

    // and then we just return the pointer to that new header
    return (void *)(header + 1);
  }

  // we firstly want to see if we can somehow split the memory after the header
  // into a smaller size if its big enough
  // firstly it needs to be bigger then the size we wanted + a new header
  if (header->size > size + BLOCK_SIZE + MIN_ALLOC_SIZE) {
    // here we want to use the MIN_ALLOC_SIZE so the block cant be like 1 byte
    // big because we never ever would need that
    split_block(header, size);
  }

  // after checking if we can split the block we can actually giving the memory
  // to the user back and setting the headers to used instead of free
  header->free = 0;

  // so here we now need to check if the first and last free element in the
  // linked list are the same, with the same meaning our header we found and if
  // thats the case we can set it both to null and thats done then
  if (first_free == header && last_free == header) {
    first_free = NULL;
    last_free = NULL;
  } else {
    // now else we have to check if we have a prev if thats the case we can just
    // do our normal linked list stuff else we can just the the next of the
    // header to the first free element
    if (header->prev_free) {
      header->prev_free->next_free = header->next_free;

    } else {
      first_free = header->next_free;
    }
    // and we do the same with the last free element in the list
    if (header->next_free) {
      header->next_free->prev_free = header->prev_free;
    } else {
      last_free = header->prev_free;
    }
  }

  // here we also have to unlock the mutex before returning
  pthread_mutex_unlock(&global_lock);

  return (void *)(header + 1);
}

// here we will now have our free function and its implementation
void free_memory(void *block) {
  // this function will basically just check if we have a block then if we have
  // one we will set its value to free and we also clear everything in the
  // memory of the block

  // first thing we have to do is of course lock the mutex
  pthread_mutex_lock(&global_lock);

  // checking if the given block is actually valid
  if (!block) {
    printf("There was an error freeing the memory because the given block was "
           "a falsey value %p\n",
           block);
    pthread_mutex_unlock(&global_lock);
    return;
  }

  // finding out what the header before the block is
  block_header *found_header = ((block_header *)block) - 1;

  // then we can set the free value of the found header to 1 to indicate that
  // its free now
  found_header->free = 1;
  // with that we also have to put the found header into our free linked list
  found_header->next_free = NULL;
  found_header->prev_free = last_free;
  // if we have a last value then we reconnect stuff else if there is no last
  // free we know either something bad happened or we dont have a linked list
  // yet and everything is NULL
  if (last_free) {
    last_free->next_free = found_header;
  } else {
    first_free = found_header;
  }
  last_free = found_header;

  // before we can actually unlock the mutex we have to merge the block
  // together which are next to this one if they are free
  merge_block(found_header);

  // last thing we do is call our memset to our block because it provides extra
  // security and we also clear the stuff if we merged
  memset(block, 0, found_header->size);

  // only then can we unlock the mutex
  pthread_mutex_unlock(&global_lock);
}

// we also need a function which will free all the memory we use in our heap
// we have to do this in order to avoid memory leaks since we ourselfs dont
// really have a choice except calling free on everything
void release_memory() { sbrk(0 - INIT_SIZE); }

// our actuall implementation for our find_first_free function we defined at the
// top
block_header *find_first_free(size_t size) {
  // this is basically just a function which goes through our linked list and
  // tries to find a header which is free and has the right size we want
  block_header *current = first_free;

  while (current) {
    if (current->free && current->size >= size) {
      return current;
    }

    current = current->next_free;
  }

  return NULL;
}

void split_block(block_header *header, size_t size) {
  // here we need to make a new block and we dont need to check anything with
  // ifs because we do that outside (hopefully)

  // first we have to make a new header which is the size of the rest memory we
  // have after our memory size we need
  // here we make the header to an actuall adress with char pointer conversion,
  // I dont really know why we do it it confuses me a bit but thats just how it
  // works
  // next we add the BLOCK_SIZE and the size we need so we are at the position
  // of the new header
  block_header *new_header =
      (block_header *)((char *)header + BLOCK_SIZE + size);
  // then we give this header all the attributes we need to give it
  new_header->size = header->size - size - BLOCK_SIZE;
  new_header->free = 1;
  // and we also have to put it into the linked list of free elements
  // meaning we dont have to add the new header into the linked list of not free
  // elements we can just add it at the end of the free linked list
  new_header->next_free = NULL;
  new_header->prev_free = last_free;
  if (last_free) {
    last_free->next_free = new_header;
  } else {
    first_free = new_header;
  }
  last_free = new_header;

  header->size = size;
}

void merge_block(block_header *header) {
  // I needed to rewrite this function to check the next headers in the memory
  // not in my linked list since in there are only free elements

  // firstly we can get the next in memory by just doing some pointer
  // arithmitics or however you spell that
  block_header *next_in_memory =
      (block_header *)((char *)header + BLOCK_SIZE + header->size);

  // we also need to know where our heap ends to see if the header we got is
  // actually still in our memory
  void *heap_end = sbrk(0);

  // now we check if our next header is actually in the heap still and we can
  // also check if the next header is free
  if ((void *)next_in_memory < heap_end && next_in_memory->free) {
    // if thats the case we can merge it together
    if (next_in_memory->prev_free) {
      next_in_memory->prev_free->next_free = next_in_memory->next_free;
    } else {
      first_free = next_in_memory->next_free;
    }

    if (next_in_memory->next_free) {
      next_in_memory->next_free->prev_free = next_in_memory->prev_free;
    } else {
      last_free = next_in_memory->prev_free;
    }

    // after we are done removing the next in memory from the linked list we can
    // just absorb it into our header
    header->size += next_in_memory->size + BLOCK_SIZE;
  }
}
