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
    texture default_diffuse_texture;
    texture default_specular_texture;
    texture default_normal_texture;

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
b8 load_cube_texture(const char* name, const char texture_names[6][TEXTURE_NAME_MAX_LENGTH], texture* t);
void destroy_texture(texture* texture);
b8 process_texture_reference(const char* name, texture_type type, i8 reference_diff, b8 auto_release, b8 skip_load, u32* out_texture_id);

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
                renderer_texture_destroy(t);    // destroy it
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
    // TODO: check against other default texture names?
    if (strings_equali(name, DEFAULT_TEXTURE_NAME)) {  // case insensitive string comparison
        KWARN("texture_system_acquire called for default textur. use texture_system_get_default_texture for texture 'default'.");
        return &state_ptr->default_texture;
    }

    u32 id = INVALID_ID;
    // NOTE: increments reference count, or creates new entry
    if (!process_texture_reference(name, TEXTURE_TYPE_2D, 1, auto_release, false, &id)) {
        KERROR("texture_system_acquire failed to obtain a new texture id.");
        return 0;
    }

    return &state_ptr->registered_textures[id];
}

texture* texture_system_acquire_cube(const char* name, b8 auto_release) {
    // return default texture, but warn about it since this should be returned via get_default_texture()
    // TODO: check against other default texture names?
    if (strings_equali(name, DEFAULT_TEXTURE_NAME)) {
        KWARN("texture_system_acquire_cube called for default texture. Use texture_system_get_default_texture for texture 'default'.");
        return &state_ptr->default_texture;
    }

    u32 id = INVALID_ID;
    // NOTE: increments reference count, or creates new entry
    if (!process_texture_reference(name, TEXTURE_TYPE_CUBE, 1, auto_release, false, &id)) {
        KERROR("texture_system_acquire_cube failed to obtain a new texture id.");
        return 0;
    }

    return &state_ptr->registered_textures[id];
}

texture* texture_system_acquire_writeable(const char* name, u32 width, u32 height, u8 channel_count, b8 has_transparency) {
    u32 id = INVALID_ID;
    // NOTE: wrapped textures are never auto released because it means that their
    // resources are created and managed somewhere within the renderer internals
    if (!process_texture_reference(name, TEXTURE_TYPE_2D, 1, false, true, &id)) {
        KERROR("texture_system_aquire_writeable failed to obtain a new texture id.");
        return 0;
    }

    texture* t = &state_ptr->registered_textures[id];
    t->id = id;
    t->type = TEXTURE_TYPE_2D;
    string_ncopy(t->name, name, TEXTURE_NAME_MAX_LENGTH);
    t->width = width;
    t->height = height;
    t->channel_count = channel_count;
    t->generation = INVALID_ID;
    t->flags |= has_transparency ? TEXTURE_FLAG_HAS_TRANSPARENCY : 0;
    t->flags |= TEXTURE_FLAG_IS_WRITEABLE;
    t->internal_data = 0;
    renderer_texture_create_writeable(t);
    return t;
}

// release the texture from memory
void texture_system_release(const char* name) {
    // ignore release requests for the default arrays
    // TODO: check against other default texture names as well?
    if (strings_equali(name, DEFAULT_TEXTURE_NAME)) {  // case insensitive string comparison
        return;
    }

    u32 id = INVALID_ID;
    // NOTE: decrement the reference count
    if (!process_texture_reference(name, TEXTURE_TYPE_2D, -1, false, false, &id)) {
        KERROR("texture_system_release failed to release texture '%s' properly.", name);
    }
}

texture* texture_system_wrap_internal(const char* name, u32 width, u32 height, u8 channel_count, b8 has_transparency, b8 is_writeable, b8 register_texture, void* internal_data) {
    u32 id = INVALID_ID;
    texture* t = 0;
    if (register_texture) {
        // NOTE: wrapped textures are never auto released because it means that their
        // resources are created and managed somewhere within the renderer internals
        if (!process_texture_reference(name, TEXTURE_TYPE_2D, 1, false, true, &id)) {
            KERROR("texture_system_wrap_internal failed to obtain a new texture id.");
            return 0;
        }
    } else {
        t = kallocate(sizeof(texture), MEMORY_TAG_TEXTURE);
        // KTRACE("texture_system_wrap_internal created texture '%s', but not registering, resulting in an allocation. It is up to the caller to free this memory.", name);
    }

    t->id = id;
    t->type = TEXTURE_TYPE_2D;
    string_ncopy(t->name, name, TEXTURE_NAME_MAX_LENGTH);
    t->width = width;
    t->height = height;
    t->channel_count = channel_count;
    t->generation = INVALID_ID;
    t->flags |= has_transparency ? TEXTURE_FLAG_HAS_TRANSPARENCY : 0;
    t->flags |= is_writeable ? TEXTURE_FLAG_IS_WRITEABLE : 0;
    t->flags |= TEXTURE_FLAG_IS_WRAPPED;
    t->internal_data = internal_data;
    return t;
}

b8 texture_system_set_internal(texture* t, void* internal_data) {
    if (t) {
        t->internal_data = internal_data;
        t->generation++;
        return true;
    }
    return false;
}

b8 texture_system_resize(texture* t, u32 width, u32 height, b8 regenerate_internal_data) {
    if (t) {
        if (!(t->flags & TEXTURE_FLAG_IS_WRITEABLE)) {
            KWARN("texture_system_resize should not be called on textures that are not writeable.");
            return false;
        }
        t->width = width;
        t->height = height;
        // only allow this for writeable textures that are not wrapped. wrapped textures can call
        // texture_system_set_internal then call this function to get the above parameter updates
        // and a generation update
        if (!(t->flags & TEXTURE_FLAG_IS_WRAPPED) && regenerate_internal_data) {
            // regenerate internals for the new size
            renderer_texture_resize(t, width, height);
            return false;
        }
        t->generation++;
        return true;
    }
    return false;
}

#define RETURN_TEXT_PTR_OR_NULL(texture, func_name)                                              \
    if (state_ptr) {                                                                             \
        return &texture;                                                                         \
    }                                                                                            \
    KERROR("%s called before texture system initialization! Null pointer returned.", func_name); \
    return 0;

texture* texture_system_get_default_texture() {
    RETURN_TEXT_PTR_OR_NULL(state_ptr->default_texture, "texture_system_get_default_texture");
}

texture* texture_system_get_default_diffuse_texture() {
    RETURN_TEXT_PTR_OR_NULL(state_ptr->default_diffuse_texture, "texture_system_get_default_diffuse_texture");
}

texture* texture_system_get_default_specular_texture() {
    RETURN_TEXT_PTR_OR_NULL(state_ptr->default_specular_texture, "texture_system_get_default_specular_texture");
}

texture* texture_system_get_default_normal_texture() {
    RETURN_TEXT_PTR_OR_NULL(state_ptr->default_normal_texture, "texture_system_get_default_normal_texture");
}

b8 create_default_textures(texture_system_state* state) {
    // NOTE: create default texture, a 256x256 blue/white checkerboard pattern
    // this is done in code to eliminate asset dependencies
    // KTRACE("Creating default texture...");
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
    state->default_texture.flags = 0;
    state->default_texture.type = TEXTURE_TYPE_2D;

    // create the texture
    renderer_texture_create(pixels, &state->default_texture);

    // manually set the texture generation to invalid since this is a default texture
    state->default_texture.generation = INVALID_ID;

    // diffuse texture
    // KTRACE("Creating default diffuse textrue...");
    u8 diff_pixels[16 * 16 * 4];
    // default diffuse map is all white
    kset_memory(diff_pixels, 255, sizeof(u8) * 16 * 16 * 4);
    string_ncopy(state->default_diffuse_texture.name, DEFAULT_DIFFUSE_TEXTURE_NAME, TEXTURE_NAME_MAX_LENGTH);
    state->default_diffuse_texture.width = 16;
    state->default_diffuse_texture.height = 16;
    state->default_diffuse_texture.channel_count = 4;
    state->default_diffuse_texture.generation = INVALID_ID;
    state->default_diffuse_texture.flags = 0;
    state->default_diffuse_texture.type = TEXTURE_TYPE_2D;
    renderer_texture_create(diff_pixels, &state->default_diffuse_texture);
    // manually set the texture generation to invalid since this is a default texture
    state->default_diffuse_texture.generation = INVALID_ID;

    // specular texture
    // KTRACE("Creating default specular texture...");
    u8 spec_pixels[16 * 16 * 4];
    // default spec map is black (no specular)
    kset_memory(spec_pixels, 0, sizeof(u8) * 16 * 16 * 4);
    string_ncopy(state->default_specular_texture.name, DEFAULT_SPECULAR_TEXTURE_NAME, TEXTURE_NAME_MAX_LENGTH);
    state->default_specular_texture.width = 16;
    state->default_specular_texture.height = 16;
    state->default_specular_texture.channel_count = 4;
    state->default_specular_texture.generation = INVALID_ID;
    state->default_specular_texture.flags = 0;
    state->default_specular_texture.type = TEXTURE_TYPE_2D;
    renderer_texture_create(spec_pixels, &state->default_specular_texture);
    // manually set the texture generation to invalid since this is a default texture
    state->default_specular_texture.generation = INVALID_ID;

    // normal texture
    // KTRACE("Creating default normal texture...");
    u8 normal_pixels[16 * 16 * 4];  // w * h * chennels
    kset_memory(normal_pixels, 0, sizeof(u8) * 16 * 16 * 4);

    // each pixel
    for (u64 row = 0; row < 16; ++row) {
        for (u64 col = 0; col < 16; ++col) {
            u64 index = (row * 16) + col;
            u64 index_bpp = index * channels;
            // set blue, z-axis by default and alpha
            normal_pixels[index_bpp + 0] = 128;
            normal_pixels[index_bpp + 1] = 128;
            normal_pixels[index_bpp + 2] = 255;
            normal_pixels[index_bpp + 3] = 255;
        }
    }

    string_ncopy(state->default_normal_texture.name, DEFAULT_NORMAL_TEXTURE_NAME, TEXTURE_NAME_MAX_LENGTH);
    state->default_normal_texture.width = 16;
    state->default_normal_texture.height = 16;
    state->default_normal_texture.channel_count = 4;
    state->default_normal_texture.generation = INVALID_ID;
    state->default_normal_texture.flags = 0;
    state->default_normal_texture.type = TEXTURE_TYPE_2D;
    renderer_texture_create(normal_pixels, &state->default_normal_texture);
    // manually set the texture generation to invalid since this is a default texture
    state->default_normal_texture.generation = INVALID_ID;

    return true;
}

// for destroying the textures created to be defaults
void destroy_default_textures(texture_system_state* state) {
    if (state) {
        destroy_texture(&state->default_texture);
        destroy_texture(&state->default_diffuse_texture);
        destroy_texture(&state->default_specular_texture);
        destroy_texture(&state->default_normal_texture);
    }
}

b8 load_cube_textures(const char* name, const char texture_names[6][TEXTURE_NAME_MAX_LENGTH], texture* t) {
    u8* pixels = 0;
    u64 image_size = 0;
    for (u8 i = 0; i < 6; ++i) {
        image_resource_params params;
        params.flip_y = false;

        resource img_resource;
        if (!resource_system_load(texture_names[i], RESOURCE_TYPE_IMAGE, &params, &img_resource)) {
            KERROR("load_cube_textures() - Failed to load image resource for texture '%s'", texture_names[i]);
            return false;
        }

        image_resource_data* resource_data = img_resource.data;
        if (!pixels) {
            t->width = resource_data->width;
            t->height = resource_data->height;
            t->channel_count = resource_data->channel_count;
            t->flags = 0;
            t->generation = 0;
            // take a copy of the name
            string_ncopy(t->name, name, TEXTURE_NAME_MAX_LENGTH);

            image_size = t->width * t->height * t->channel_count;
            // NOTE: no need for transparency in cube maps, so not checking for it

            pixels = kallocate(sizeof(u8) * image_size * 6, MEMORY_TAG_ARRAY);
        } else {
            // verify that all textures are the same size
            if (t->width != resource_data->width || t->height != resource_data->height || t->channel_count != resource_data->channel_count) {
                KERROR("load_cube_textures - All textures must be the same resolution and bit depth.");
                kfree(pixels, sizeof(u8) * image_size * 6, MEMORY_TAG_ARRAY);
                pixels = 0;
                return false;
            }
        }

        // copy to the relevant portion of the array
        kcopy_memory(pixels + image_size * i, resource_data->pixels, image_size);

        // clean up data
        resource_system_unload(&img_resource);
    }

    // Acquire internal texture resources and upload to gpu
    renderer_texture_create(pixels, t);

    kfree(pixels, sizeof(u8) * image_size * 6, MEMORY_TAG_ARRAY);
    pixels = 0;

    return true;
}

b8 load_texture(const char* texture_name, texture* t) {
    image_resource_params params;
    params.flip_y = true;

    resource img_resource;
    if (!resource_system_load(texture_name, RESOURCE_TYPE_IMAGE, &params, &img_resource)) {
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
    temp_texture.flags = has_transparency ? TEXTURE_FLAG_HAS_TRANSPARENCY : 0;

    // aquire internal texture resources and upload to GPU
    renderer_texture_create(resource_data->pixels, &temp_texture);

    // take a copy of the old texture
    texture old = *t;  // derefence and save to old

    // assign the temp texture to the pointer
    *t = temp_texture;  // dereference and save the temp texture

    // destroy the old texture - to avoid leaking resources
    renderer_texture_destroy(&old);

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
    renderer_texture_destroy(t);

    kzero_memory(t->name, sizeof(char) * TEXTURE_NAME_MAX_LENGTH);
    kzero_memory(t, sizeof(texture));
    t->id = INVALID_ID;
    t->generation = INVALID_ID;
}

b8 process_texture_reference(const char* name, texture_type type, i8 reference_diff, b8 auto_release, b8 skip_load, u32* out_texture_id) {
    *out_texture_id = INVALID_ID;
    if (state_ptr) {
        texture_reference ref;
        if (hashtable_get(&state_ptr->registered_texture_table, name, &ref)) {
            // if the reference count starts off at zero, one of two things can be true. if incrementing references,
            // this means the entry is new. if decrementing, then the texture doesnt exist if not auto releasing
            if (ref.reference_count == 0 && reference_diff > 0) {
                if (reference_diff > 0) {
                    // this can only be changed the first time a texture is loaded
                    ref.auto_release = auto_release;
                } else {
                    if (ref.auto_release) {
                        KWARN("Tried to release non-existent texture: '%s'", name);
                        return false;
                    } else {
                        KWARN("Tried to release a texture where autorelease=false, but references was already 0.");
                        // still count this as a success but warn about it
                        return true;
                    }
                }
            }

            ref.reference_count += reference_diff;

            // take a copy of the name since it would be wiped out if destroyed,
            // (as passed in name is generally a pointer to the actual texture's name)
            char name_copy[TEXTURE_NAME_MAX_LENGTH];

            // if decrementing this means a release
            if (reference_diff < 0) {
                // check if the reference count has reached 0. if it has, and the reference is set to auto release,
                // destroy the texture
                if (ref.reference_count == 0 && ref.auto_release) {
                    texture* t = &state_ptr->registered_textures[ref.handle];

                    // destroy/reset texture
                    destroy_texture(t);

                    // reset the reference
                    ref.handle = INVALID_ID;
                    ref.auto_release = false;
                    // KTRACE("Released texture '%s'., Texture unloaded because reference count=0 and auto_release=true.", name_copy);
                } else {
                    // KTRACE("Released texture '%s', now has a reference count of '%i' (auto_release=%s).", name_copy, ref.reference_count, ref.auto_release ? "true" : "false");
                }
            } else {
                // incrementing. check if the handle is new or not
                if (ref.handle == INVALID_ID) {
                    // this means that no texture exists here. find a free index first
                    u32 count = state_ptr->config.max_texture_count;

                    for (u32 i = 0; i < count; ++i) {
                        if (state_ptr->registered_textures[i].id == INVALID_ID) {
                            // a free slot has been found. use its index as the handle
                            ref.handle = i;
                            *out_texture_id = i;
                            break;
                        }
                    }

                    // an empty slot was not found, bleat about it and boot out
                    if (*out_texture_id == INVALID_ID) {
                        KFATAL("process_texture_reference - Texture system cannot hold anymore textures. Adjust configuration to allow more.");
                        return false;
                    } else {
                        texture* t = &state_ptr->registered_textures[ref.handle];
                        t->type = type;
                        // create new texture
                        if (skip_load) {
                            // KTRACE("Load skipped for texture '%s'. This is expected behaviour.");
                        } else {
                            if (type == TEXTURE_TYPE_CUBE) {
                                char texture_names[6][TEXTURE_NAME_MAX_LENGTH];

                                // +X,-X,+Y,-Y,+Z,-Z in cubemap space, ehic is LH y-down
                                string_format(texture_names[0], "%s_r", name);  // right texture
                                string_format(texture_names[1], "%s_l", name);  // left texture
                                string_format(texture_names[2], "%s_u", name);  // up texture
                                string_format(texture_names[3], "%s_d", name);  // down texture
                                string_format(texture_names[4], "%s_f", name);  // front texture
                                string_format(texture_names[5], "%s_b", name);  // back texture

                                if (!load_cube_textures(name, texture_names, t)) {
                                    *out_texture_id = INVALID_ID;
                                    KERROR("Failed to load cube texture '%s'.", name);
                                    return false;
                                }
                            } else {
                                if (!load_texture(name, t)) {
                                    *out_texture_id = INVALID_ID;
                                    KERROR("Failed to load texture '%s'.", name);
                                    return false;
                                }
                            }
                            t->id = ref.handle;
                        }
                        // KTRACE("Texture '%s' does not yet exist. Created, and ref_count is now %i.", name, ref.reference_count);
                    }
                } else {
                    *out_texture_id = ref.handle;
                    // KTRACE("Texture '%s' already exists, ref_count increased to %i.", name, ref.reference_count);
                }
            }

            // either way, update the entry
            hashtable_set(&state_ptr->registered_texture_table, name_copy, &ref);
            return true;
        }

        // NOTE: this would only happen in the event something went wrong with the state
        KERROR("process_texture_reference failed to acquire id for name '%s'. INVALID_ID returned.", name);
        return false;
    }

    KERROR("process_texture_reference called before texture system is initialized.");
    return false;
}