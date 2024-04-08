#pragma once

#include "resources/resource_types.h"

// store the configuration settings for the resource system
typedef struct resource_system_config {
    u32 max_loader_count;
    // the relative base path for assets
    char* asset_base_path;
} resource_system_config;

// store the info for the resource loaders
typedef struct resource_loader {
    u32 id;                                                                              // internal id
    resource_type type;                                                                  // enum, type like image, text, ect
    const char* custom_type;                                                             // if type is custom, can provide a string to name it or describe it
    const char* type_path;                                                               // base path for the given type of resource, like materials will be in a materials folder, textures in a texture folder, ect
    b8 (*load)(struct resource_loader* self, const char* name, resource* out_resource);  // pointer to a function to load a resource
    void (*unload)(struct resource_loader* self, resource* resource);                    // pointer to a function to unload a resource
} resource_loader;

// initialize the resource system, will be handled in 2 steps, first stem the pointer to the state is not passed in, and the memory requirememnts are obtained
// it actually initializes the second time the function is called, after memory has been allocated to hold the system
b8 resource_system_initialize(u64* memory_requirement, void* state, resource_system_config config);

// shut down the resource system
void resource_system_shutdown(void* state);

// exported function to register a loader
KAPI b8 resource_system_register_loader(resource_loader loader);

// load functions
// used by the system
KAPI b8 resource_system_load(const char* name, resource_type type, resource* out_resource);
// used by user code
KAPI b8 resource_system_load_custom(const char* name, const char* custom_type, resource* out_resource);

// unload
KAPI void resource_system_unload(resource* resource);

// getter for the resource system base path
KAPI const char* resource_system_base_path();