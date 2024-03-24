#pragma once

#include "defines.h"

// store info for linear allocators, for linear memory allocation
typedef struct linear_allocator {
    u64 total_size;  // total size to allocate in bytes
    u64 allocated;   // the total amount to allocate for each element, know how far to move the pointer keeping track of the current position
    void* memory;    // a pointer to the actual block of memory itself
    b8 owns_memory;  // set depending on what is put into the create method -- may pull memory from another allocater, or arena(believe this is a large allocation to pass out in smaller allocations) - help avoid calls to malloc
} linear_allocator;

// create a linear allocator- pass in the total size(how big a block to allocate,or if passing too, how much), a pointer to already allocated memory of passing in, and a pointer to the resulting allocator
KAPI void linear_allocator_create(u64 total_size, void* memory, linear_allocator* out_allocator);

// destroy a linear allocator, just takes a pointer to the allocator
KAPI void linear_allocator_destroy(linear_allocator* allocator);

// allocate memory from a linear allocator, pass in a pointer to the linear allocator to allocate memory from, and the size of the memory to be allocated
KAPI void* linear_allocator_allocate(linear_allocator* allocator, u64 size);

// free all the memory of the allocator passed in, just pass in the pointer to a linear allocator
KAPI void linear_allocator_free_all(linear_allocator* allocator);