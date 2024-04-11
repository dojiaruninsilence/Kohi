#pragma once

#include "defines.h"

// @brief a data structure to be used alongside and allocator for dynamic memory allocation.
// tracks free ranges of memory
typedef struct freelist {
    // @brief the internal state of the free list
    void* memory;
} freelist;

// @brief creates a new free list or obtains the memory requirement for one. call twice; once passing 0 to memory to obtain memory requirement,
// and a second time passing an allocated block of memory
// @param total_size the total size in bytes that the free list should track.
// @param memory_requirement a pointer to hold memory requirement for the free list itself
// @param memory 0, or a pre-allocated block of memory for the free list to use
// @param out_list a pointer to hold the created free list
KAPI void freelist_create(u64 total_size, u64* memory_requirement, void* memory, freelist* out_list);

// @brief destroys the provided list
// @param list the list to be destroyed
KAPI void freelist_destroy(freelist* list);

// @brief attempts to find a free block of memory of the given size.
// @param list a pointer to the list to search.
// @param size the size to allocate
// @param out_offset a pointer to hold the offset to the allocated memory
// @return b8 true if a block of memory was found and allocated; otherwise false
KAPI b8 freelist_allocate_block(freelist* list, u64 size, u64* out_offset);

// @brief attempts to free a block of memory at the given offset, and of the given size. can faile if invalid data is passed.
// @param list a pointer to the list to be free from
// @param size the size to be freed
// @param offset the offset to free at
// @return b8 true if successful; otherwise false. false should be treated as an error
KAPI b8 freelist_free_block(freelist* list, u64 size, u64 offset);

// @brief attempts to resize the provided freelist to the given size. internal data is copied to the new block of memory.
// the old block must be freed after this call.
// NOTE: new size must be greater than the existing size of the given list
// NOTE: should be called twice; once to query the memory requirement (passing new_memory=0),
// and the second time to actually resize the list
// @param list a pointer to the list to be resized.
// @param memory_requirement a pointer to hold the amount of memory required for the resize
// @param new_memory the new block of state memory. set to 0 if only querying memory requirement
// @param new_size the new size. must be greater than the size of the provided list
// @param out_old_memory a pointer to hold the old block of memory so that it may be freed after this call
// @return true on success, otherwise false
KAPI b8 freelist_resize(freelist* list, u64* memory_requirement, void* new_memory, u64 new_size, void** out_old_memory);

// @brief clears the free list
// @param list the list to be cleared
KAPI void freelist_clear(freelist* list);

// @brief returns the amount of free space in this list. NOTE: since this has to iterate the entire internal list,
// this can be an expensive operation. use sparingly
// @param list a pointer to the list to obtain from
// @return the amount of free space in bytes
KAPI u64 freelist_free_space(freelist* list);