#include "resource_system.h"

#include "core/logger.h"
#include "core/kstring.h"

// known resource loaders
#include "resources/loaders/text_loader.h"
#include "resources/loaders/binary_loader.h"
#include "resources/loaders/image_loader.h"
#include "resources/loaders/material_loader.h"

// store the state for the resource system
typedef struct resource_system_state {
    resource_system_config config;        // configuration settings
    resource_loader* registered_loaders;  // pointer to an array of loaders(img, txt, ect)
} resource_system_state;

// pointer to the state for internal use
static resource_system_state* state_ptr = 0;

// internal function that does the work of actually loading the files
b8 load(const char* name, resource_loader* loader, resource* out_resource);

// initialize the resource system, will be handled in 2 steps, first stem the pointer to the state is not passed in, and the memory requirememnts are obtained
// it actually initializes the second time the function is called, after memory has been allocated to hold the system
b8 resource_system_initialize(u64* memory_requirement, void* state, resource_system_config config) {
    if (config.max_loader_count == 0) {
        KFATAL("resource_system_initialize failed bacause config.max_loader_count==0.");
        return false;
    }

    // dereference the memory requirement and set to the size of a resource loader times the count of loaders plus the size of the state of the resource system
    *memory_requirement = sizeof(resource_system_state) + (sizeof(resource_loader) * config.max_loader_count);

    // if this is the first call, boot out after getting the required memory
    if (!state) {
        return true;
    }

    // pass through a pointer to the state, and the configuration settings
    state_ptr = state;
    state_ptr->config = config;

    void* array_block = state + sizeof(resource_system_state);  // set the pointer in the linear allocation for the array of loaders
    state_ptr->registered_loaders = array_block;                // save the pointer to the array in the state

    // invalidate all loaders
    u32 count = config.max_loader_count;
    for (u32 i = 0; i < count; i++) {
        state_ptr->registered_loaders[i].id = INVALID_ID;
    }

    // NOTE: auto register known loader types here
    resource_system_register_loader(text_resource_loader_create());
    resource_system_register_loader(binary_resource_loader_create());
    resource_system_register_loader(image_resource_loader_create());
    resource_system_register_loader(material_resource_loader_create());

    KINFO("Resource system initialized with base path '%s'.", config.asset_base_path);

    return true;
}

// shut down the resource system
void resource_system_shutdown(void* state) {
    if (state_ptr) {
        state_ptr = 0;
    }
}

// exported function to register a loader
b8 resource_system_register_loader(resource_loader loader) {
    if (state_ptr) {
        u32 count = state_ptr->config.max_loader_count;
        // ensure that no other loaders for the given type already exist
        for (u32 i = 0; i < count; ++i) {                            // iterater through the array of loaders
            resource_loader* l = &state_ptr->registered_loaders[i];  // get a pointer to the loader at index i
            if (l->id != INVALID_ID) {                               // if there is a loader there
                if (l->type == loader.type) {                        // if the input loader type matched the loader at index i
                    KERROR("resource_system_register_loader - Loader of type %d already exists and will not be registered.", loader.type);
                    return false;
                } else if (loader.custom_type && string_length(loader.custom_type) > 0 && strings_equali(l->custom_type, loader.custom_type)) {  // or if it matches another custom type
                    KERROR("resource_system_register_loader - Loader of custom type %s already exists and will not be registered.", loader.custom_type);
                    return false;
                }
            }
        }
        for (u32 i = 0; i < count; ++i) {  // iterate through the array of loaders looking for the first empty slot - one thats id is invalid id
            if (state_ptr->registered_loaders[i].id == INVALID_ID) {
                state_ptr->registered_loaders[i] = loader;  // copy the loader into the array
                state_ptr->registered_loaders[i].id = i;    // id becomes index i
                KTRACE("Loader registered.");
                return true;
            }
        }
    }

    // it failed
    return false;
}

// load functions
// used by the system
b8 resource_system_load(const char* name, resource_type type, resource* out_resource) {
    if (state_ptr && type != RESOURCE_TYPE_CUSTOM) {
        // Select loader.
        u32 count = state_ptr->config.max_loader_count;
        for (u32 i = 0; i < count; ++i) {
            resource_loader* l = &state_ptr->registered_loaders[i];
            if (l->id != INVALID_ID && l->type == type) {
                return load(name, l, out_resource);
            }
        }
    }

    out_resource->loader_id = INVALID_ID;
    KERROR("resource_system_load - No loader for type %d was found.", type);
    return false;
}

// used by user code
b8 resource_system_load_custom(const char* name, const char* custom_type, resource* out_resource) {
    if (state_ptr && custom_type && string_length(custom_type) > 0) {  // verify there is a state, a type was passed in, and the type had some value
        // select the loader
        u32 count = state_ptr->config.max_loader_count;
        for (u32 i = 0; i < count; ++i) {                                                                                 // iterate through all of the loaders
            resource_loader* l = &state_ptr->registered_loaders[i];                                                       // get a pointer to the loader in the array at index i
            if (l->id != INVALID_ID && l->type == RESOURCE_TYPE_CUSTOM && strings_equali(l->custom_type, custom_type)) {  // if the id is valid, the type is custom, and the passed in custom type matches the type at index i
                return load(name, l, out_resource);                                                                       // load the resource and return it
            }
        }
    }

    out_resource->loader_id = INVALID_ID;
    KERROR("resource_system_load_custom - No loader for type %s was found.", custom_type);
    return false;
}

// unload
void resource_system_unload(resource* resource) {
    if (state_ptr && resource) {
        if (resource->loader_id != INVALID_ID) {
            resource_loader* l = &state_ptr->registered_loaders[resource->loader_id];  // get a pointer to the resource at the proper index
            if (l->id != INVALID_ID && l->unload) {
                l->unload(l, resource);  // call unload function
            }
        }
    }
}

// getter for the resource system base path
const char* resource_system_base_path() {
    if (state_ptr) {
        return state_ptr->config.asset_base_path;
    }

    KERROR("resource_system_base_path was called before initialization, returning an empty string");
    return "";
}

// internal function that does the work of actually loading the files
b8 load(const char* name, resource_loader* loader, resource* out_resource) {
    if (!name || !loader || !loader->load || !out_resource) {  // verify that all of the proper data was passed in
        if (out_resource) {
            out_resource->loader_id = INVALID_ID;
        }
        return false;
    }

    // store the id in the resulting struct
    out_resource->loader_id = loader->id;
    return loader->load(loader, name, out_resource);  // use load function to load the loader, and return it
}