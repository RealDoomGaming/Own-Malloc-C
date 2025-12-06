#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "mm.h"

// here we define when we want to ues sbrk and mmap, like for which amount of
// byte anything below 128KB we can use sbrk but aynthing above that we have to
// ues mmap
#define THRESHOLD (128 * 1024)

// we have a global variable for our first and last element in the linked list
// so we can manage it easier
header_t *first = NULL, *last = NULL;
// we also need a global lock so in the future we can lock the header struct so
// only 1 thread can use it else it could cause problems if multiple threads
// tried allocating to the same memory at the same time
pthread_mutex_t global_lock;

header_t *get_first_free_header(size_t size);

void *memory_alloc(size_t size) {
  // we first look if size is even valid
  if (!size) {
    perror("Memory Allocation");
    printf("There was an error allocating the memory because the given size "
           "was null or 0\n");
    return NULL;
  }

  // after validating everything we need to see if we have a struct in the list
  // with the right amount of size for our use case
  // also we need to lock the mutex here before we try to do any memory
  // managment stuff
  pthread_mutex_lock(&global_lock);
  // then we need to get a struct which is free but also has the right size for
  // our use
  header_t *header = get_first_free_header(size);
  // if we got back a free block we can return it to the user
  if (header) {
    // we need to mark this block now as not free but we also need to unlock the
    // mutex and return this header
    header->free = 0;
    pthread_mutex_unlock(&global_lock);
    // what we return here is a bit complicated but its basically just the
    // memory adress next to the header in the heap which is basically the same
    // size as the header and because we convert it to a void pointer it just
    // points there and
    return (void *)(header + 1);
  }

  // if we didnt find anything we can just get rid of the pointer
  free(header);

  // now we also need to do something once we conclude that we dont the right
  // header file for the right size this will always happen at the beginning
  // because the linked list doesnt have any elements so the while loop in our
  // helper function is instantly null

  // we need to get the toal size of the size we want + our header since the
  // header will alwasy be before our actual memory
  size_t size_total = sizeof(header_t) + size;
  // since we dont have anything in the linked list we also now need to request
  // memory with either the sbrk or with the mmap sbrk is for lighter workloads
  // and is faster but bigger workloads will performe better with mmap

  // we will have to define a void pointer to the future block we allocate
  // memory for up here so we can use it in either way and only have to define
  // it once
  void *block;
  if (size_total <= THRESHOLD) {
    // if the size is smaller then 128KB then we use sbrk
    block = sbrk(size_total);

  } else {
    // else if the size is bigger then 128KB then we us mmap
    // now this mmap might look a bit more confusing then the sbrk function so
    // let me explain everything NULL -> we could give it a dedicated memory
    // adress but we tell the kernel that they can pick whatever memory adress
    // they want
    // size_total -> length of the allocated memory
    // PROT_READ | PROT_WRITE -> this is the memory acess permission, so we tell
    // the kernel we want to read and write stuff into the memoy
    // MAP_PRIVATE | MAP_ANONYMOUS -> this tells the kernel how you are mapping
    // and how it will behave so this means that we want a private chunck of
    // memory which is not tied to any file
    // -1 -> this is just the file descriptor (fd), normally mmap maps files but
    // when you dont want that and use MAP_ANONYMOUS then we have to pass -1
    // here
    // 0 -> this is the offset into the file, again since we dont map a file we
    // put 0 here
    block = mmap(NULL, size_total, PROT_READ | PROT_WRITE,
                 MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  }

  // if there was an error either getting memory from the os with mmap or with
  // sbrk then we unlock the mutex and return;
  if (block == MAP_FAILED || block == (void *)-1) {
    pthread_mutex_unlock(&global_lock);

    perror("Memory Allocation");
    printf("Allocating memory failed because either the sbrk or the mmap "
           "failed\n");
    return NULL;
  }

  // after we know that we were able to allocate memory we can create the new
  // header and set all its variables
  header_t *new_header = block;
  new_header->size = size;
  new_header->free = 0;
  new_header->next = NULL;

  if (!first) {
    first = new_header;
  }
  if (last) {
    last->next = new_header;
  }
  last = new_header;
  pthread_mutex_unlock(&global_lock);

  return (void *)(new_header + 1);
}

void free_memory(void *block) {
  // first we have to check if the current block even exists
  if (!block) {
    perror("Free Memory");
    printf("There was an error freeing the memory because the given block "
           "wasnt valid");
    return;
  }

  // firstly before anything we have to lock it so nothing bad happens
  pthread_mutex_lock(&global_lock);

  // so here we just make the block to a header_t pointer so then we can do - 1
  // on it and the pointer moves back header_t amount of bytes from the block
  // pointer
  header_t *header = ((header_t *)block) - 1;
  // then we have the actual header for this block

  // then we can see if the block is at the end of the heap by firstly getting
  // the programbreak we achive this by using the sbrk but with a 0 as a
  // parameter
  void *programbreak = sbrk(0);

  // now if our header is the last one in the heap we release memory to the os
  // but we also have to change our linked list
  if ((char *)block + header->size == programbreak) {
    // if we know for sure that this is the last in the heap
    // we first have to change the linked list accordingly
    // and then we have to release the memory to the os (I will explain how that
    // works later)

    if (first == last) {
      // if we only have 1 header in the linked list then the first and the last
      // will be the same
      first = NULL;
      last = NULL;
    } else {
      // if the first and last elements of the linked list arent the same then
      // we have to just go through the entire linked list so we can find the
      // element before the last header and set it to the last
      header_t *current = first;

      while (current) {
        // so if we find out the next one is the last one we can just set the
        // current on to the last one and also set the next to null since we
        // remove that
        if (current->next == last) {
          last = current;
          current->next = NULL;
        }

        // else we continue to go though the linked list
        current = current->next;
      }
    }

    // then lastly we have to release the memory to the os, this works though //
    // calling sbrk and giving a negative value so we release that amount //
    // going from the end of the heap so for us we will take the size of the
    // block we have minus the size of // our header_t struct and then go into
    // minus by subtracting all that from (the programmbreak)
    sbrk(0 - sizeof(block) - sizeof(header_t));

    // then lastly we return and also unlock the mutex
    pthread_mutex_unlock(&global_lock);
    return;
  }

  // then if its not the case that the header is the last in the heap we can
  // just set it to free
  header->free = 1;
  // but also what we have to do is clear the memory in the block after the
  // header
  memset(block, 0, header->size);
  // and we alos have to unlock the mutex and return
  pthread_mutex_unlock(&global_lock);
  return;
}

header_t *get_first_free_header(size_t size) {
  // the function to getting the first free header with the right size

  // we get the first header in the linked list
  header_t *current = first;
  // and then we loop through every element of the linked list until we either
  // find the right one or we return with null
  while (current) {
    // if we find a struct which is currently free and has the size which is
    // bigger or equal to what we need we return the currently selected struct
    // from the linked list else we continue and go to the next element
    if (current->free && current->size >= size) {
      return current;
    }

    current = current->next;
  }

  return NULL;
}
