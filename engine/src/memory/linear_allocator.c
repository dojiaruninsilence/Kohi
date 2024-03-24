#include "linear_allocator.h"

#include "core/kmemory.h"
#include "core/logger.h"

// create a linear allocator- pass in the total size(how big a block to allocate,or if passing too, how much), a pointer to already allocated memory of passing in, and a pointer to the resulting allocator
void linear_allocator_create(u64 total_size, void* memory, linear_allocator* out_allocator) {
    if (out_allocator) {                           // make sure that an allocator pointer has been defined
        out_allocator->total_size = total_size;    // pass through the total size
        out_allocator->allocated = 0;              // ensure that allocated is zero bofore passing any data in
        out_allocator->owns_memory = memory == 0;  // if memory is zero, meaning memory is being allocated, not passed in, then set to true, other wise it is false
        if (memory) {                              // if memory is being passed in
            out_allocator->memory = memory;        // then pass that memory in to memory
        } else {
            out_allocator->memory = kallocate(total_size, MEMORY_TAG_LINEAR_ALLOCATOR);  // allocate memory for the linear allocator, pass in total size for size and tag as linear allocator
        }
    }
}

// destroy a linear allocator, just takes a pointer to the allocator
void linear_allocator_destroy(linear_allocator* allocator) {
    if (allocator) {                                                                       // make sure that an allocator pointer has been defined
        allocator->allocated = 0;                                                          // ensure that allocated is zero
        if (allocator->owns_memory && allocator->memory) {                                 // if owns memory is true and there is actually memory
            kfree(allocator->memory, allocator->total_size, MEMORY_TAG_LINEAR_ALLOCATOR);  // free the memory passing in the memory, the size of the memory and the linear allocator tag
        }
        allocator->memory = 0;           // then reset memory
        allocator->total_size = 0;       // and reset the total size
        allocator->owns_memory = false;  // set owns memory to the default false
    }
}

// allocate memory from a linear allocator, pass in a pointer to the linear allocator to allocate memory from, and the size of the memory to be allocated - it will return a pointer to the memory allocated
void* linear_allocator_allocate(linear_allocator* allocator, u64 size) {
    if (allocator && allocator->memory) {                                                                           // if there is an allocator passed in and that alocator actually has memory
        if (allocator->allocated + size > allocator->total_size) {                                                  // if the total of the memory already allocated plus the size of memory wanting to be allocated is bigger than the total size
            u64 remaining = allocator->total_size - allocator->allocated;                                           // get a total of how much memory is left by subtracting the amount allocated from the total size
            KERROR("linear allocator allocate - tried to allocate %lluB, only %lluB remaining.", size, remaining);  // throw an error - %llu is the token for a 64 bit unsigned int
            return 0;                                                                                               // return of zero means the application failed
        }

        void* block = ((u8*)allocator->memory) + allocator->allocated;  // create a pointer to a block of memory. takes the pointer in memory and shifts it by the amount of memory allocated already
        allocator->allocated += size;                                   // increase the memory allocated by the size of memory wanting to be added
        return block;
    }

    // if there is no allocator, or no memory in the allocator
    KERROR("linear allocator allocate - provided allocator is not initialized");
    return 0;  // has failed
}

// free all the memory of the allocator passed in, just pass in the pointer to a linear allocator
void linear_allocator_free_all(linear_allocator* allocator) {
    if (allocator && allocator->memory) {                        // if there is an allocator passed in and that alocator actually has memory
        allocator->allocated = 0;                                // move the pointer back to the beggining
        kzero_memory(allocator->memory, allocator->total_size);  // use our function to free the memory, give it the address of the memory and the size of it
    }
}