////////////////////////////////////////////////////////////////////////////////
/*                 (๑＞◡＜๑)  Malloc Challenge!!  (◍＞◡＜◍)                   */
////////////////////////////////////////////////////////////////////////////////

//
// Welcome to Malloc Challenge!! Your job is to invent a smart malloc algorithm.
//
// Rules:
//
// 1. Your job is to implement my_malloc(), my_free() and my_initialize().
//   *  my_initialize() is called only once at the beginning of each challenge.
//      You can initialize the memory allocator.
//   *  my_malloc(size) is called every time an object is allocated. In this
//      challenge, |size| is guaranteed to be a multiple of 8 bytes and meets
//      8 <= size <= 4000.
//   * my_free(ptr) is called every time an object is freed.
//   * Additionally, my_finalize() is called only once at the end of each
//   challenge,
//     so you can use this function for doing some clean ups if you want.
// 2. The only library functions you can use in my_malloc() and my_free() are
//    mmap_from_system() and munmap_to_system().
//   *  mmap_from_system(size) allocates |size| bytes from the system. |size|
//      needs to be a multiple of 4096 bytes. mmap_from_system(size) is a
//      system call and heavy. You are expected to minimize the call of
//      mmap_from_system(size) by reusing the returned
//      memory region as much as possible.
//   *  munmap_to_system(ptr, size) frees the memory region [ptr, ptr + size)
//      to the system. |ptr| and |size| need to be a multiple of 4096 bytes.
//      You are expected to free memory regions that are unused.
//   *  You are NOT allowed to use any other library functions at all, including
//      the default malloc() / free(), std:: libraries etc. This is because you
//      are implementing malloc itself -- if you use something that may use
//      malloc internally, it will result in an infinite recurion.
// 3. simple_malloc(), simple_free() and simple_initialize() in simple_malloc.c
//    are an example of straightforward implementation.
//    Your job is to invent a smarter malloc algorithm than the simple malloc.
// 4. There are five challenges (Challenge 1, 2, 3, 4 and 5). Each challenge
//    allocates and frees many objects with different patterns. Your malloc
//    is evaluated by two criteria.
//   *  [Speed] How faster your malloc finishes the challange compared to
//      the simple malloc.
//   *  [Memory utilization] How much your malloc is memory efficient.
//      This is defined as (S1 / S2), where S1 is the total size of objects
//      allocated at the end of the challange and S2 is the total size of
//      mmap_from_system()ed regions at the end of the challenge. You can
//      improve the memory utilization by decreasing memory fragmentation and
//      reclaiming unused memory regions to the system with munmap_to_system().
// 5. This program works on Linux and Mac but not on Windows. If you don't have
//    Linux or Mac, you can use Google Cloud Shell (See
//    https://docs.google.com/document/d/1TNu8OfoQmiQKy9i2jPeGk1DOOzSVfbt4RoP_wcXgQSs/edit#).
// 6. You need to specify an '-lm' option to compile this program.
//   *  gcc malloc_challenge.c -lm
//   *  clang malloc_challenge.c -lm
//
// Enjoy! :D
//

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/time.h>

typedef struct simple_metadata_t {
  size_t size;
  struct simple_metadata_t *next;
} simple_metadata_t;

// The global information of the simple malloc.
//   *  |free_head| points to the first free slot.
//   *  |dummy| is a dummy free slot (only used to make the free list
//      implementation simpler).
typedef struct simple_heap_t {
  simple_metadata_t *free_head;
  simple_metadata_t dummy;
} simple_heap_t;

simple_heap_t simple_heap;

void *mmap_from_system(size_t size);
void munmap_to_system(void *ptr, size_t size);
void simple_add_to_free_list(simple_metadata_t *metadata);
void simple_remove_from_free_list(simple_metadata_t *metadata,
                                  simple_metadata_t *prev);

// my_initialize() is called only once at the beginning of each challenge.
void my_initialize() {
  simple_heap.free_head = &simple_heap.dummy;
  simple_heap.dummy.size = 0;
  simple_heap.dummy.next = NULL;
}

// my_malloc() is called every time an object is allocated. |size| is guaranteed
// to be a multiple of 8 bytes and meets 8 <= |size| <= 4000. You are not
// allowed to use any library functions other than mmap_from_system /
// munmap_to_system.

/*
void my_malloc(size_t size) {
  my_malloc_best_fit(size);
  // return my_malloc_worst_fit(size);
}*/

void *my_malloc_best_fit(size_t size) {
  simple_metadata_t *metadata = simple_heap.free_head;
  simple_metadata_t *prev = NULL;
  // First-fit: Find the first free slot the object fits.
  size_t min_size;
  simple_metadata_t *min_prev = NULL;
  simple_metadata_t *min_metadata = NULL;
  while (metadata) {
    if (metadata->size >= size) {
      if (!min_metadata || min_size > metadata->size) {
        min_metadata = metadata;
        min_prev = prev;
        min_size = metadata->size;
      }
    }
    prev = metadata;
    metadata = metadata->next;
  }

  if (!min_metadata) {
    // There was no free slot available. We need to request a new memory region
    // from the system by calling mmap_from_system().
    //
    //     | metadata | free slot |
    //     ^
    //     metadata
    //     <---------------------->
    //            buffer_size
    size_t buffer_size = 4096;
    simple_metadata_t *metadata =
        (simple_metadata_t *)mmap_from_system(buffer_size);
    metadata->size = buffer_size - sizeof(simple_metadata_t);
    metadata->next = NULL;
    // Add the memory region to the free list.
    simple_add_to_free_list(metadata);
    // Now, try simple_malloc() again. This should succeed.
    return my_malloc_best_fit(size);
  }

  // |ptr| is the beginning of the allocated object.
  //
  // ... | metadata | object | ...
  //     ^          ^
  //     metadata   ptr
  void *ptr = min_metadata + 1;
  size_t remaining_size = min_metadata->size - size;
  min_metadata->size = size;
  // Remove the free slot from the free list.
  simple_remove_from_free_list(min_metadata, min_prev);

  if (remaining_size > sizeof(simple_metadata_t)) {
    // Create a new metadata for the remaining free slot.
    //
    // ... | metadata | object | metadata | free slot | ...
    //     ^          ^        ^
    //     metadata   ptr      new_metadata
    //                 <------><---------------------->
    //                   size       remaining size
    simple_metadata_t *new_metadata = (simple_metadata_t *)((char *)ptr + size);
    new_metadata->size = remaining_size - sizeof(simple_metadata_t);
    new_metadata->next = NULL;
    // Add the remaining free slot to the free list.
    simple_add_to_free_list(new_metadata);
  }
  return ptr;
}

void *my_malloc_worst_fit(size_t size) {
  simple_metadata_t *metadata = simple_heap.free_head;
  simple_metadata_t *prev = NULL;
  // First-fit: Find the first free slot the object fits.
  size_t max_size = 0;
  simple_metadata_t *max_prev = NULL;
  simple_metadata_t *max_metadata = NULL;
  while (metadata) {
    if (metadata->size >= size && metadata->size > max_size) {
      max_metadata = metadata;
      max_prev = prev;
      max_size = metadata->size;
    }
    prev = metadata;
    metadata = metadata->next;
  }

  if (!max_metadata) {
    // There was no free slot available. We need to request a new memory
    // region from the system by calling mmap_from_system().
    //
    //     | metadata | free slot |
    //     ^
    //     metadata
    //     <---------------------->
    //            buffer_size
    size_t buffer_size = 4096;
    simple_metadata_t *metadata =
        (simple_metadata_t *)mmap_from_system(buffer_size);
    metadata->size = buffer_size - sizeof(simple_metadata_t);
    metadata->next = NULL;
    // Add the memory region to the free list.
    simple_add_to_free_list(metadata);
    // Now, try simple_malloc() again. This should succeed.
    return my_malloc_worst_fit(size);
  }

  // |ptr| is the beginning of the allocated object.
  //
  // ... | metadata | object | ...
  //     ^          ^
  //     metadata   ptr
  void *ptr = max_metadata + 1;
  size_t remaining_size = max_metadata->size - size;
  max_metadata->size = size;
  // Remove the free slot from the free list.
  simple_remove_from_free_list(max_metadata, max_prev);

  if (remaining_size > sizeof(simple_metadata_t)) {
    // Create a new metadata for the remaining free slot.
    //
    // ... | metadata | object | metadata | free slot | ...
    //     ^          ^        ^
    //     metadata   ptr      new_metadata
    //                 <------><---------------------->
    //                   size       remaining size
    simple_metadata_t *new_metadata = (simple_metadata_t *)((char *)ptr + size);
    new_metadata->size = remaining_size - sizeof(simple_metadata_t);
    new_metadata->next = NULL;
    // Add the remaining free slot to the free list.
    simple_add_to_free_list(new_metadata);
  }
  return ptr;
}

// my_free() is called every time an object is freed.  You are not allowed to
// use any library functions other than mmap_from_system / munmap_to_system.
void my_free(void *ptr) {
  simple_metadata_t *metadata = (simple_metadata_t *)ptr - 1;
  // Add the free slot to the free list.
  simple_add_to_free_list(metadata);
}

void my_finalize() {
  // Implement here!
  
  /*
  simple_metadata_t *metadata = simple_heap.free_head;
  simple_metadata_t *prev = NULL;
  while (metadata) {  
    prev = metadata;
    metadata = metadata->next;
    printf("%d %d\n", prev1, metadata);
    if (prev  + prev->size == metadata) {
      prev->size += metadata->size;
      prev->next = metadata->next;
      metadata = prev->next;
    }
  }*/
}

void test() {
  // Implement here!
  assert(1 == 1); /* 1 is 1. That's always true! (You can remove this.) */
}
