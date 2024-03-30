#pragma once

#include "renderer/renderer_types.inl"

// store the texture system configurations
typedef struct texture_system_config {
    u32 max_texture_count;  // max amount of textures the system can store
} texture_system_config;

// so we can change this in one place
#define DEFAULT_TEXTURE_NAME "default"

// initialize the texture system- it takes in a pointer to how much memory is required, a pointer to a block of memory, and the texture system configurations
// this is one of the 2 stage functions, 1st time call with a zero for the block of memory(state), and it will calculate the memory requirements
// after allocating memory for the system, call again and pass in the block of memory - returns a bool for success checking
b8 texture_system_initialize(u64* memory_requirement, void* state, texture_system_config config);

// shutdown the texture system
void texture_system_shutdown(void* state);

// pass in a name, and it checks if that texture has been loaded, if it has not been loaded it attempts to load it. and returns a pointer to the texture if successful
// auto release - if a texture is no longer in use it will be released to free up memory
texture* texture_system_aquire(const char* name, b8 auto_release);

// release the texture from memory
void texture_system_release(const char* name);

// get a pointer to the default texture
texture* texture_system_get_default_texture();
