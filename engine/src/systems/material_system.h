#pragma once

#include "defines.h"

#include "resources/resource_types.h"

#define DEFAULT_MATERIAL_NAME "default"

// where we store the configuration settings for the material system
typedef struct material_system_config {
    u32 max_material_count;  // all it holds for now, is the maximum number of material allowed for now
} material_system_config;

// initialize the material system -- this is a 2 step function, call the first time without the pointer to the block of memory
// it will return the amount of memory the system will require to run, memory must then be allocated,
// and then passed in to the function and ran again, this time actually initializing the system
b8 material_system_initialize(u64* memory_requirement, void* state, material_system_config config);

// shut down the material system
void material_system_shutdown(void* state);

// acquire a material by name from file - assumes that there is a material with that name
material* material_system_acquire(const char* name);

// acquire a material system from a config - from code instead of a file
material* material_system_acquire_from_config(material_config config);

// release a material by name
void material_system_release(const char* name);

// get the default material
material* material_system_get_default();