#include "dynamic_allocator.h"

#include "core/kmemory.h"
#include "core/logger.h"
#include "containers/freelist.h"

// store the state of the allocator locally
typedef struct dynamic_allocator_state {
    u64 total_size;        // total size of the allocator
    freelist list;         // a freelist for the allocator, makes it dynamic
    void* freelist_block;  // pointer to the memory for the free list
    void* memory_block;    // pointer to the actual memory to be allocated out
} dynamic_allocator_state;

b8 dynamic_allocator_create(u64 total_size, u64* memory_requirement, void* memory, dynamic_allocator* out_allocator) {
    if (total_size < 1) {
        KERROR("dynamic_allocator_create cannot have a total_size of 0. Create failed.");
        return false;
    }
    if (!memory_requirement) {
        KERROR("dynamic_allocator_create requires memory_requirement to exist.  Create failed.");
        return false;
    }
    u64 freelist_requirement = 0;
    // grab the memory requirement for the free list first
    freelist_create(total_size, &freelist_requirement, 0, 0);

    // the block of memory will include the state, the freelist, and all of the memory to be allocated out, all in one block
    *memory_requirement = freelist_requirement + sizeof(dynamic_allocator_state) + total_size;

    // if only obtaining requirement, boot out
    if (!memory) {
        return true;
    }

    // memory layout:
    // state
    // freelist block
    // memory block
    out_allocator->memory = memory;                          // the entire block
    dynamic_allocator_state* state = out_allocator->memory;  // state will be the fist block chunked out
    state->total_size = total_size;
    state->freelist_block = (void*)(out_allocator->memory + sizeof(dynamic_allocator_state));  // point the freelist to its block of memory
    state->memory_block = (void*)(state->freelist_block + freelist_requirement);

    // actually create the freel list
    freelist_create(total_size, &freelist_requirement, state->freelist_block, &state->list);

    kzero_memory(state->memory_block, total_size);  // zero out only the memory that will be allocated out
    return true;
}

b8 dynamic_allocator_destroy(dynamic_allocator* allocator) {
    if (allocator) {
        dynamic_allocator_state* state = allocator->memory;
        freelist_destroy(&state->list);
        kzero_memory(state->memory_block, state->total_size);
        state->total_size = 0;
        allocator->memory = 0;
        return true;
    }

    KWARN("dynamic_allocator_destroy requires a pointer to an allocator. Destroy failed.");
    return false;
}

void* dynamic_allocator_allocate(dynamic_allocator* allocator, u64 size) {
    if (allocator && size) {
        dynamic_allocator_state* state = allocator->memory;
        u64 offset = 0;
        // attempt to allocate from the freelist
        if (freelist_allocate_block(&state->list, size, &offset)) {
            // use that offset against the base memory block to get the block
            void* block = (void*)(state->memory_block + offset);
            return block;  // the now allocated block of memory
        } else {
            KERROR("dynamic_allocator_allocate no blocks of memory large enough to allocate from.");
            u64 available = freelist_free_space(&state->list);
            KERROR("Requested size: %llu, total space available: %llu", size, available);
            // TODO: report fragmentation?
            return 0;
        }
    }

    KERROR("dynamic_allocator_allocate requires a valid allocator and size.");
    return 0;
}

b8 dynamic_allocator_free(dynamic_allocator* allocator, void* block, u64 size) {
    if (!allocator || !block || !size) {
        KERROR("dynamic_allocator_free requires both a valid allocator (0x%p) and a block (0x%p) to be freed.", allocator, block);
        return false;
    }

    dynamic_allocator_state* state = allocator->memory;
    if (block < state->memory_block || block > state->memory_block + state->total_size) {
        void* end_of_block = (void*)(state->memory_block + state->total_size);
        KERROR("dynamic_allocator_free trying to release block (0x%p) outside of allocator range (0x%p)-(0x%p)", block, state->memory_block, end_of_block);
        return false;
    }
    u64 offset = (block - state->memory_block);
    if (!freelist_free_block(&state->list, size, offset)) {
        KERROR("dynamic_allocator_free failed.");
        return false;
    }

    return true;
}

u64 dynamic_allocator_free_space(dynamic_allocator* allocator) {
    dynamic_allocator_state* state = allocator->memory;
    return freelist_free_space(&state->list);
}