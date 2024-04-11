#pragma once

#include "defines.h"


// @brief the dynamic allocator structure
typedef struct dynamic_allocator {
    // @brief the allocated memory block for this allocator to use
    void* memory;
} dynamic_allocator;

// @brief creates a new dynamic allocator. should be called twice; once to obtain the memory amount required (passing memory=0),
// and a second time with memory being set to an allocated block
// @param total_size the total size in bytes the allocator should hold. note, this size does not include the size of the internal state
// @param memory_requirement a pointer to hold the required memory for the internal state plus the total size
// @param memory an allocated block of memory, or 0 if just getting the requirement
// @param out_allocator a pointer to hold the allocator.
// @return true on success, otherwise false
KAPI b8 dynamic_allocator_create(u64 total_size, u64* memory_requirement, void* memory, dynamic_allocator* out_allocator);

// @brief destroys the given allocator
// @param allocator a pointer to the allocator to be destroyed
// @return true on success, otherwise false
KAPI b8 dynamic_allocator_destroy(dynamic_allocator* allocator);

// @brief allocates the given amount of memory from the provided allocator
// @param allocator a pointer to the allocator to allocate from
// @param size the amount in bytes to be allocated
// @return the allocated block of memory unless this operation fails, then 0
KAPI void* dynamic_allocator_allocate(dynamic_allocator* allocator, u64 size);

// @brief frees the given block of memory
// @param allocator a pointer to the allocator to free from
// @param block the block to be freed. must have been allocated by the provided allocator
// @param size the size of the block
// @return true on success; otherwise false
KAPI b8 dynamic_allocator_free(dynamic_allocator* allocator, void* block, u64 size);

// @brief obtains the amount of free space left in the provided allocator
// @param allocator a pointer to the allocator to be examined
// @return the amount of free space in bytes
KAPI u64 dynamic_allocator_free_space(dynamic_allocator* allocator);