#include "texture_system.h"

#include "core/logger.h"
#include "core/kstring.h"
#include "core/kmemory.h"
#include "containers/hashtable.h"

#include "renderer/renderer_frontend.h"

#include "systems/resource_system.h"

typedef struct texture_system_state {
    texture_system_config config;  // hang on to a copy of the config state
    texture default_texture;
    texture default_specular_texture;

    // array of registered textures - works in tandem with registered_texture_table
    texture* registered_textures;

    // hashtable for texture lookups - works in tandem with registered_textures
    hashtable registered_texture_table;
} texture_system_state;

typedef struct texture_reference {
    u64 reference_count;
    u32 handle;
    b8 auto_release;
} texture_reference;

void create_texture(texture* t) {
    kzero_memory(t, sizeof(texture));
    t->generation = INVALID_ID;
}

// hold on to the system state with a static pointer
static texture_system_state* state_ptr = 0;

// private function forward declarations
b8 create_default_textures(texture_system_state* state);
void destroy_default_textures(texture_system_state* state);
b8 load_texture(const char* texture_name, texture* t);
void destroy_texture(texture* texture);

// initialize the texture system- it takes in a pointer to how much memory is required, a pointer to a block of memory, and the texture system configurations
// this is one of the 2 stage functions, 1st time call with a zero for the block of memory(state), and it will calculate the memory requirements
// after allocating memory for the system, call again and pass in the block of memory - returns a bool for success checking
b8 texture_system_initialize(u64* memory_requirement, void* state, texture_system_config config) {
    if (config.max_texture_count == 0) {
        KFATAL("texture_system_initialize - config.max_texture_count must be > 0.");
        return false;
    }

    // block of memory will contain state structure, then a block for the array, then a block for the hashtable
    u64 struct_requirement = sizeof(texture_system_state);                                 // contain the state structure
    u64 array_requirement = sizeof(texture) * config.max_texture_count;                    // contain the array of textures
    u64 hashtable_requirement = sizeof(texture_reference) * config.max_texture_count;      // contain everything for the texture hashtable
    *memory_requirement = struct_requirement + array_requirement + hashtable_requirement;  // add them all together to get the total memory needed for the texture system store that in dereferenced memory requirement

    if (!state) {     // if this was the first pass and the state was not input
        return true;  // boot out here
    }

    state_ptr = state;           // set state pointer to the block of memory
    state_ptr->config = config;  // pass through the configuration infos

    // the array block is after the state. already allocated, so just set the pointer
    void* array_block = state + struct_requirement;  // move the pointer the size of the state structure
    state_ptr->registered_textures = array_block;    // give access of the texture array to the state

    // hashtable block is after the array
    void* hashtable_block = array_block + array_requirement;  // shift the pointer the size of the texture array

    // create a hashtable for texture lookups - the size of each element will be the size of a texture reference, the count is the max count, pass it the memory address, is not a pointer type, and lastly the address for the hashtable
    hashtable_create(sizeof(texture_reference), config.max_texture_count, hashtable_block, false, &state_ptr->registered_texture_table);

    // fill the hashtable with invalid references to use as a default
    texture_reference invalid_ref;
    invalid_ref.auto_release = false;
    invalid_ref.handle = INVALID_ID;  // primary reason for needing default values
    invalid_ref.reference_count = 0;
    hashtable_fill(&state_ptr->registered_texture_table, &invalid_ref);

    // invalidate all textures in the array
    u32 count = state_ptr->config.max_texture_count;
    for (u32 i = 0; i < count; ++i) {  // iterate through the texture array
        state_ptr->registered_textures[i].id = INVALID_ID;
        state_ptr->registered_textures[i].generation = INVALID_ID;
    }

    // create default textures for use in the system
    create_default_textures(state_ptr);

    return true;
}

// shutdown the texture system
void texture_system_shutdown(void* state) {
    if (state_ptr) {  // if a texture system exists
        // destroy all loaded textures
        for (u32 i = 0; i < state_ptr->config.max_texture_count; ++i) {  // iterate through the entire texture array
            texture* t = &state_ptr->registered_textures[i];
            if (t->generation != INVALID_ID) {  // if there is a registered texture at index i
                renderer_destroy_texture(t);    // destroy it
            }
        }

        destroy_default_textures(state_ptr);  // destroy the default textures

        state_ptr = 0;  // reset the state pointer
    }
}

// pass in a name, and it checks if that texture has been loaded, if it has not been loaded it attempts to load it. and returns a pointer to the texture if successful
// auto release - if a texture is no longer in use it will be released to free up memory
texture* texture_system_acquire(const char* name, b8 auto_release) {
    // return default texture, but warn about it since this should be returned via get_default_texturez()
    if (strings_equali(name, DEFAULT_TEXTURE_NAME)) {  // case insensitive string comparison
        KWARN("texture_system_acquire called for default textur. use texture_system_get_default_texture for texture 'default'.");
        return &state_ptr->default_texture;
    }

    texture_reference ref;  // store a copy to use locally

    if (state_ptr && hashtable_get(&state_ptr->registered_texture_table, name, &ref)) {  // if the state pointer exists and hashtable get succeeds
        // this can only be changed the first time a texture is loaded
        if (ref.reference_count == 0) {       // if this is the first time that the texture has been loaded
            ref.auto_release = auto_release;  // then set the auto release
        }
        ref.reference_count++;  // increment the reference count
        if (ref.handle == INVALID_ID) {
            // this means that no texture exists here. find a free index first
            u32 count = state_ptr->config.max_texture_count;               // get the size of the textuer array
            texture* t = 0;                                                // define a texture to store a new texture in
            for (u32 i = 0; i < count; ++i) {                              // loop through all of the registered textures
                if (state_ptr->registered_textures[i].id == INVALID_ID) {  // if a texture id is invalid
                    // a free slot has been found. use its index as the handle
                    ref.handle = i;  // set the handle to i
                    t = &state_ptr->registered_textures[i];
                    break;
                }
            }

            // make sure an empty slot was actually found
            if (!t || ref.handle == INVALID_ID) {
                KFATAL("texture_system_acquire - texture system cannot hold anymore textures. adjust configurations to allow more.");
                return 0;
            }

            // create a new texture
            if (!load_texture(name, t)) {
                KERROR("failed to load texture '%s'.", name);
                return 0;
            }

            // also use the handle as the texture id
            t->id = ref.handle;
            KTRACE("Texture '%s' does not yet exist. created, and ref_count is now %i.", name, ref.reference_count);
        } else {
            KTRACE("Texture '%s' already exists, ref_count increased to %i.", name, ref.reference_count);
        }

        // update the entry
        hashtable_set(&state_ptr->registered_texture_table, name, &ref);
        return &state_ptr->registered_textures[ref.handle];
    }

    // NOTE: this would only happen in the event something went wrong with the state
    KERROR("texture_system_acquire failed to acquire texture '%s'. Null pointer will be returned.", name);
    return 0;
}

// release the texture from memory
void texture_system_release(const char* name) {
    // ignore release requests for the default arrays
    if (strings_equali(name, DEFAULT_TEXTURE_NAME)) {  // case insensitive string comparison
        return;
    }

    texture_reference ref;  // store a copy to use locally

    if (state_ptr && hashtable_get(&state_ptr->registered_texture_table, name, &ref)) {  // if the state pointer exists and hashtable get succeeds
        if (ref.reference_count == 0) {                                                  // if this is the first time that the texture has been loaded
            KWARN("Tried to release non-existant texture: '%s'", name);
            return;
        }

        // take a copy of the name since it will be wiped out by destroy
        // (as passed in name is generally a pointer to the actual texture's name)
        char name_copy[TEXTURE_NAME_MAX_LENGTH];
        string_ncopy(name_copy, name, TEXTURE_NAME_MAX_LENGTH);

        ref.reference_count--;  // decrement the reference count
        if (ref.reference_count == 0 && ref.auto_release) {
            texture* t = &state_ptr->registered_textures[ref.handle];

            // destroy/reset texture
            destroy_texture(t);

            // reset the reference
            ref.handle = INVALID_ID;
            ref.auto_release = false;

            KTRACE("Released texture '%s', Texture unloaded because reference count = 0 and auto_release = true.", name_copy);
        } else {
            KTRACE("Released texture '%s', now has a reference count of '%i'(auto_release = %s).", name_copy, ref.reference_count, ref.auto_release ? "true" : "false");
        }

        // update the entry
        hashtable_set(&state_ptr->registered_texture_table, name_copy, &ref);
    } else {
        KERROR("texture_system_acquire failed to release texture '%s'.", name);
        return;
    }
}

// get a pointer to the default texture
texture* texture_system_get_default_texture() {
    if (state_ptr) {
        return &state_ptr->default_texture;
    }

    KERROR("texture_system_get_default_texture called before texture system initialization! Null pointer returned.");
    return 0;
}

texture* texture_system_get_default_specular_texture() {
    if (state_ptr) {
        return &state_ptr->default_specular_texture;
    }

    KERROR("texture_system_get_default_specular_texture called before the texture system initialization! Null pointer returned");
    return 0;
}

b8 create_default_textures(texture_system_state* state) {
    // NOTE: create default texture, a 256x256 blue/white checkerboard pattern
    // this is done in code to eliminate asset dependencies
    KTRACE("Creating default texture...");
    const u32 tex_dimension = 256;                                  // size the texture will be in width and height
    const u32 channels = 4;                                         // channels
    const u32 pixel_count = tex_dimension * tex_dimension;          // w * h
    u8 pixels[262144];                                              // dimensions * channels
    kset_memory(pixels, 255, sizeof(u8) * pixel_count * channels);  // fill the array with 255 which makes it white

    // each pixel
    for (u64 row = 0; row < tex_dimension; ++row) {      // iterate through all the rows
        for (u64 col = 0; col < tex_dimension; ++col) {  // iterate though all the columns
            u64 index = (row * tex_dimension) + col;
            u64 index_bpp = index * channels;
            if (row % 2) {                      // if the row is even
                if (col % 2) {                  // and the col is even
                    pixels[index_bpp + 0] = 0;  // flip the r channel to zero
                    pixels[index_bpp + 1] = 0;  // flip the g channel to zero
                }
            } else {                            // if the row is odd
                if (!(col % 2)) {               // and the col is odd
                    pixels[index_bpp + 0] = 0;  // flip the r channel to zero
                    pixels[index_bpp + 1] = 0;  // flip the g channel to zero
                }
            }
        }
    }

    string_ncopy(state->default_texture.name, DEFAULT_TEXTURE_NAME, TEXTURE_NAME_MAX_LENGTH);
    state->default_texture.width = tex_dimension;
    state->default_texture.height = tex_dimension;
    state->default_texture.channel_count = 4;
    state->default_texture.generation = INVALID_ID;
    state->default_texture.has_transparency = false;

    // create the texture
    renderer_create_texture(pixels, &state->default_texture);

    // manually set the texture generation to invalid since this is a default texture
    state_ptr->default_texture.generation = INVALID_ID;

    // specular texture
    KTRACE("Creating default specular texture...");
    u8 spec_pixels[16 * 16 * 4];
    // default spec map is black (no specular)
    kset_memory(spec_pixels, 0, sizeof(u8) * 16 * 16 * 4);
    string_ncopy(state->default_specular_texture.name, DEFAULT_SPECULAR_TEXTURE_NAME, TEXTURE_NAME_MAX_LENGTH);
    state->default_specular_texture.width = 16;
    state->default_specular_texture.height = 16;
    state->default_specular_texture.channel_count = 4;
    state->default_specular_texture.generation = INVALID_ID;
    state->default_specular_texture.has_transparency = false;
    renderer_create_texture(spec_pixels, &state->default_specular_texture);
    // manually set the texture generation to invalid since this is a default texture
    state->default_specular_texture.generation = INVALID_ID;

    return true;
}

// for destroying the textures created to be defaults
void destroy_default_textures(texture_system_state* state) {
    if (state) {
        destroy_texture(&state->default_texture);
        destroy_texture(&state->default_specular_texture);
    }
}

b8 load_texture(const char* texture_name, texture* t) {
    resource img_resource;
    if (!resource_system_load(texture_name, RESOURCE_TYPE_IMAGE, &img_resource)) {
        KERROR("Failed to load image resource for texture '%s'", texture_name);
        return false;
    }

    image_resource_data* resource_data = img_resource.data;

    // use a temporary texture to load into
    texture temp_texture;
    temp_texture.width = resource_data->width;
    temp_texture.height = resource_data->height;
    temp_texture.channel_count = resource_data->channel_count;  // if the image has the wrong channel count overwrite it here

    u32 current_generation = t->generation;  // in case the texture has already been loaded, hang on to the id number
    t->generation = INVALID_ID;

    u64 total_size = temp_texture.width * temp_texture.height * temp_texture.channel_count;
    // check for transparency
    b32 has_transparency = false;
    for (u64 i = 0; i < total_size; i += temp_texture.channel_count) {  // iterate over all of the pixels(count by number of channels)
        u8 a = resource_data->pixels[i + 3];                            // copy the a channel of the pixel
        if (a < 255) {                                                  // if the a channel is less than 255 the pixel is at least partially transparent
            has_transparency = true;
            break;
        }
    }

    // take a copy of the name
    string_ncopy(temp_texture.name, texture_name, TEXTURE_NAME_MAX_LENGTH);
    temp_texture.generation = INVALID_ID;
    temp_texture.has_transparency = has_transparency;

    // aquire internal texture resources and upload to GPU
    renderer_create_texture(resource_data->pixels, &temp_texture);

    // take a copy of the old texture
    texture old = *t;  // derefence and save to old

    // assign the temp texture to the pointer
    *t = temp_texture;  // dereference and save the temp texture

    // destroy the old texture - to avoid leaking resources
    renderer_destroy_texture(&old);

    if (current_generation == INVALID_ID) {  // if this is the first one
        t->generation = 0;                   // set generation to 0
    } else {
        t->generation = current_generation + 1;  // set generation to an increment generation by one
    }

    // clean up the data
    resource_system_unload(&img_resource);
    return true;
}

void destroy_texture(texture* t) {
    // clean up backend resources
    renderer_destroy_texture(t);

    kzero_memory(t->name, sizeof(char) * TEXTURE_NAME_MAX_LENGTH);
    kzero_memory(t, sizeof(texture));
    t->id = INVALID_ID;
    t->generation = INVALID_ID;
}