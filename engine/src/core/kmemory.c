#include "kmemory.h"

#include "core/logger.h"
#include "core/kstring.h"
#include "platform/platform.h"
#include "memory/dynamic_allocator.h"

// TODO: custom string library
#include <string.h>
#include <stdio.h>

struct memory_stats {
    u64 tolal_allocated;
    u64 tagged_allocations[MEMORY_TAG_MAX_TAGS];
};

// this will become more robust as the engine grows - just an array of srtings that matches the memory tags -
static const char* memory_tag_strings[MEMORY_TAG_MAX_TAGS] = {
    "UNKNOWN    ",
    "ARRAY      ",
    "LINEAR_ALLC",
    "DARRAY     ",
    "DICT       ",
    "RING_QUEUE ",
    "BST        ",
    "STRING     ",
    "APPLICATION",
    "JOB        ",
    "TEXTURE    ",
    "MAT_INST   ",
    "RENDERER   ",
    "GAME       ",
    "TRANSFORM  ",
    "ENTITY     ",
    "ENTITY_NODE",
    "SCENE      "};

// where we will store the state information for the memory system
typedef struct memory_system_state {
    memory_system_configuration config;  // struct for defining the config setting
    struct memory_stats stats;           // store the struct for the memory stats
    u64 alloc_count;                     // keep track of the number of dynamic allocations made
    u64 allocator_memory_requirement;    // how much memory is needed for allocations
    dynamic_allocator allocator;         // store a dynamic allocator
    void* allocator_block;               // pointer to actual block of memory in the allocator
} memory_system_state;

// define a pointer to where the memory state is going to be stored -- to privately track it in the memory system
static memory_system_state* state_ptr;

// initialize the memory subsystem- pass in a pointer to the where memory reuirements for the state will be stored, and a pointer to the where the memory for the state will be, 0 for the first run to get the size requirements
b8 memory_system_initialize(memory_system_configuration config) {
    // the amount needed by the system state
    u64 state_memory_requirement = sizeof(memory_system_state);

    // figure out how much space the dynamic allocator needs
    u64 alloc_requirement = 0;
    dynamic_allocator_create(config.total_alloc_size, &alloc_requirement, 0, 0);

    // call the plaform allocator to get the memory for the whole system, including the state
    // TODO: memory alignment
    void* block = platform_allocate(state_memory_requirement + alloc_requirement, false);
    if (!block) {
        KFATAL("Memory system allocation failed and the system cannot continue.");
        return false;
    }

    // the state is in the first part of the massive block of memory
    state_ptr = (memory_system_state*)block;
    state_ptr->config = config;
    state_ptr->alloc_count = 0;  // reset the number of dynamic allocations to 0;
    state_ptr->allocator_memory_requirement = alloc_requirement;
    platform_zero_memory(&state_ptr->stats, sizeof(state_ptr->stats));  // starts by zeroing out all of the stats, in case any left over from a previous call
    // the allocator block is in the same block of memory, but after the state.
    state_ptr->allocator_block = ((void*)block + state_memory_requirement);

    // actually create the dynamic allocator
    if (!dynamic_allocator_create(
            config.total_alloc_size,
            &state_ptr->allocator_memory_requirement,
            state_ptr->allocator_block,
            &state_ptr->allocator)) {
        KFATAL("Memory system is unable to setup internal allocator. Application cannot continue.");
        return false;
    }

    KDEBUG("Memory system successfully allocated %llu bytes.", config.total_alloc_size);
    return true;
}

// shutdown the memory subsystem, just pass it the pointer to the state
void memory_system_shutdown() {
    if (state_ptr) {
        dynamic_allocator_destroy(&state_ptr->allocator);
        // free the entire block
        platform_free(state_ptr, state_ptr->allocator_memory_requirement + sizeof(memory_system_state));
    }
}

void* kallocate(u64 size, memory_tag tag) {
    if (tag == MEMORY_TAG_UNKNOWN) {
        KWARN("kallocate called using MEMORY_TAG_UNKNOWN. re-class this allocation.");  // let us know if the tag is unknown, will still be valid but we should know so we can fix it
    }

    // either allocate from the system's allocator or the os. the latter shouldnt ever really happen
    void* block = 0;
    if (state_ptr) {
        state_ptr->stats.tolal_allocated += size;          // add the size that is passed in to total allocated. size in bytes
        state_ptr->stats.tagged_allocations[tag] += size;  // add size to tagged allocated, using tag to match the proper index - how we track memory per category
        state_ptr->alloc_count++;                          // everytime memory is allocated increment the count of dynamic allocations

        block = dynamic_allocator_allocate(&state_ptr->allocator, size);
    } else {
        // if the system is not up yet, warn about it, but give memory for now
        KWARN("kallocate was called before the memory system was initialized.");
        // TODO: memory alignment
        block = platform_allocate(size, false);
    }

    if (block) {
        platform_zero_memory(block, size);
        return block;
    }

    KFATAL("kallocate failed to allocate successfully.");
    return 0;
}

void kfree(void* block, u64 size, memory_tag tag) {
    if (tag == MEMORY_TAG_UNKNOWN) {
        KWARN("kfree called using MEMORY_TAG_UNKNOWN. re class this allocation.");  // let us know if the tag is unknown, will still be valid but we should know so we can fix it
    }

    if (state_ptr) {
        state_ptr->stats.tolal_allocated -= size;          // remove the size passed in from total allocated stats
        state_ptr->stats.tagged_allocations[tag] -= size;  // remove the size passed in from tagged allocations at index of tag
        b8 result = dynamic_allocator_free(&state_ptr->allocator, block, size);

        // if the free failed, its possible this is because the allocation was made before the system had been initialized.
        // since this should absolutely be an exeption to the rule, try freeing it on the platform level. if this fails,
        // some other form of skullduggery is affoot, and we have bigger problems on our hands
        if (!result) {
            // TODO: memory alignment
            platform_free(block, false);
        }
    } else {
        // TODO: memory alignment
        platform_free(block, false);
    }
}

// the next three are easy, they just call their platform specific counterparts, passing the same values
void* kzero_memory(void* block, u64 size) {
    return platform_zero_memory(block, size);
}

void* kcopy_memory(void* dest, const void* source, u64 size) {
    return platform_copy_memory(dest, source, size);
}

void* kset_memory(void* dest, i32 value, u64 size) {
    return platform_set_memory(dest, value, size);
}

char* get_memory_usage_str() {
    const u64 gib = 1024 * 1024 * 1024;  // GB for converting values to gb if they are big enough - they go up by multiples of 1024 to be exact, not 1000
    const u64 mib = 1024 * 1024;         // MB for converting values to gb if they are big enough but not too big to be gb
    const u64 kib = 1024;                // KB for converting values to gb if they are big enough but not too big to be kb

    char buffer[8000] = "System memory use (tagged):\n";  // character buffer for formating some strings
    u64 offset = strlen(buffer);                          // has to be maintained along the way - this is the size of the string as it is now
    // loop through each of the categories and print them out each on their own lines
    for (u32 i = 0; i < MEMORY_TAG_MAX_TAGS; ++i) {
        char unit[4] = "XiB";                                              // template string, gets swapped around depending on the category
        float amount = 1.0f;                                               // will be the calculated gb, mb, kb or whatever
        if (state_ptr->stats.tagged_allocations[i] >= gib) {               // if the category's allocations are greater than or equal to gb use gb as the unit
            unit[0] = 'G';                                                 // swap the X in Xib to G
            amount = state_ptr->stats.tagged_allocations[i] / (float)gib;  // divide the allocated amount by the value of gib to get the allocated amount in GB
        } else if (state_ptr->stats.tagged_allocations[i] >= mib) {        // if the category's allocations are greater than or equal to mb use mb as the unit
            unit[0] = 'M';                                                 // swap the X in Xib to M
            amount = state_ptr->stats.tagged_allocations[i] / (float)mib;  // divide the allocated amount by the value of mib to get the allocated amount in MB
        } else if (state_ptr->stats.tagged_allocations[i] >= kib) {        // if the category's allocations are greater than or equal to kb use kb as the unit
            unit[0] = 'K';                                                 // swap the X in Xib to K
            amount = state_ptr->stats.tagged_allocations[i] / (float)kib;  // divide the allocated amount by the value of kib to get the allocated amount in KB
        } else {                                                           // else its small enough to remain in bytes
            unit[0] = 'B';                                                 // swap the X in Xib to B
            unit[1] = 0;                                                   // swap the rest of unit with nothing
            amount = (float)state_ptr->stats.tagged_allocations[i];
        }

        i32 length = snprintf(buffer + offset, 8000, "  %s: %.2f%s\n", memory_tag_strings[i], amount, unit);  // append everything to a readable string and get the length
        offset += length;                                                                                     // add new length to the offset
    }
    // calling this dynamically like this is not ideal, but since it will be used infrequently for debug stuff it should be ok
    char* out_string = string_duplicate(buffer);  // copies the string?
    return out_string;                            // have to be careful with this.  have to free this everytime that this is called
}

u64 get_memory_alloc_count() {
    if (state_ptr) {  // make sure there is a state to get a count from
        return state_ptr->alloc_count;
    }
    return 0;
}