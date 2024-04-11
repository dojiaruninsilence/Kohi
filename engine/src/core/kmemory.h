#pragma once

#include "defines.h"

// this is another are that will be added too as the engine grows
typedef enum memory_tag {  // enum is for enumeration, need to look this up as well
    // for temporary use. should be assigned one of the below or have a new tag created.
    MEMORY_TAG_UNKNOWN,  // default
    MEMORY_TAG_ARRAY,
    MEMORY_TAG_LINEAR_ALLOCATOR,
    MEMORY_TAG_DARRAY,
    MEMORY_TAG_DICT,
    MEMORY_TAG_RING_QUEUE,
    MEMORY_TAG_BST,
    MEMORY_TAG_STRING,
    MEMORY_TAG_APPLICATION,
    MEMORY_TAG_JOB,
    MEMORY_TAG_TEXTURE,
    MEMORY_TAG_MATERIAL_INSTANCE,
    MEMORY_TAG_RENDERER,
    MEMORY_TAG_GAME,
    MEMORY_TAG_TRANSFORM,
    MEMORY_TAG_ENTITY,
    MEMORY_TAG_ENTITY_NODE,
    MEMORY_TAG_SCENE,

    MEMORY_TAG_MAX_TAGS  // use this to itterate through all the tags - always has to be the last entry in the list
} memory_tag;

// @brief the configuration for the memory system
typedef struct memory_system_configuration {
    // @brief the total memory size in bytes used by the internal allocator for this system
    u64 total_alloc_size;
} memory_system_configuration;

// run twice eveytime, first to get the memory required, then second to actually initialize the system
// initialize the memory subsystem - pass in a pointer to where the memory requirements fiels is, and a pointer to where the state is going to be in memory, or a zero if getting the memory requirement
KAPI b8 memory_system_initialize(memory_system_configuration config);  // all sub systems need initializing

// shut down the memory subsystem
KAPI void memory_system_shutdown();  // and a shutdown

KAPI void* kallocate(u64 size, memory_tag tag);  // almost like a malloc - but takes in a memory_tag - and allows the engine and us to keep track

KAPI void kfree(void* block, u64 size, memory_tag tag);  // need to keep track of allocations as well as the amoust of space they are taking up

KAPI void* kzero_memory(void* block, u64 size);  // takes the block and the size of the block and zeros out the block

KAPI void* kcopy_memory(void* dest, const void* source, u64 size);  // will work the same as memcpy

KAPI void* kset_memory(void* dest, i32 value, u64 size);  // equivilent of memset

KAPI char* get_memory_usage_str();  // mostly a debug function -- will print out useful statistics to the console

KAPI u64 get_memory_alloc_count();  // a check to see how many allocations are being made