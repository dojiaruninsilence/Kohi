#pragma once

#include "renderer/renderer_types.inl"

// store the texture system configurations
typedef struct texture_system_config {
    u32 max_texture_count;  // max amount of textures the system can store
} texture_system_config;

// so we can change this in one place
#define DEFAULT_TEXTURE_NAME "default"

// @brief the default diffuse texture name
#define DEFAULT_DIFFUSE_TEXTURE_NAME "default_DIFF"

// @brief the default specular texture name
#define DEFAULT_SPECULAR_TEXTURE_NAME "default_SPEC"

// @brief the default normal texture name
#define DEFAULT_NORMAL_TEXTURE_NAME "default_NORM"

// initialize the texture system- it takes in a pointer to how much memory is required, a pointer to a block of memory, and the texture system configurations
// this is one of the 2 stage functions, 1st time call with a zero for the block of memory(state), and it will calculate the memory requirements
// after allocating memory for the system, call again and pass in the block of memory - returns a bool for success checking
b8 texture_system_initialize(u64* memory_requirement, void* state, texture_system_config config);

// shutdown the texture system
void texture_system_shutdown(void* state);

// pass in a name, and it checks if that texture has been loaded, if it has not been loaded it attempts to load it. and returns a pointer to the texture if successful
// auto release - if a texture is no longer in use it will be released to free up memory
texture* texture_system_acquire(const char* name, b8 auto_release);

// @brief attempts to acquire a cubemap texture with the given name. if it has not yet been loaded, this trigger is to load. if the texture is not
// found, a pointer to the default texture is returned. if the texture is found and loaded, its reference counter is incremented. requires textures
// with name as the base, one for each side of a cube, in the following order:
// - name_f front
// - name_f back
// - name_f up
// - name_f down
// - name_f right
// - name_f left
// for example, "skybox_f.png", "skybox_b.png", ect. where name is "skybox"
// @param name the name of the texture to find. used as a base string for actual texture names
// @param auto_release indicates if the texture should auto release when its reference count is 0. only takes effect the first time the texture is loaded
// @return a pointer to the loaded texture. can be a pointer to the default texture if not found
texture* texture_system_acquire_cube(const char* name, b8 auto_release);

// @brief attempts to acquire a writeable texture with the given name. this does not point to  nor attempt to load a texture file.
// does also increment the reference counter NOTE: writable textures are not auto released
// @param name the name of the texture to be acquired
// @param width the width of the texture in pixels
// @param height the height of the texture in pixels
// @param channel_count the number of channels in the texture (typically 4 for rgba)
// @param has_transparency indicates if the texture will have transparency
// @return a pointer to the generated texture
texture* texture_system_acquire_writeable(const char* name, u32 width, u32 height, u8 channel_count, b8 has_transparency);

// release the texture from memory
void texture_system_release(const char* name);

// @brief wraps the provided internal data in a texture stucture using the parameters provided. this is best used for when the
// render system creates internal resources and they should be passed off to the texture system. can be looked up by name via
// the acquire methods NOTE: wrapped textures are not auto released
// @param name the name of the texture
// @param width the width of the texture in pixels
// @param height the height of the texture in pixels
// @param channel_count the number of channels in the texture (typically 4 for rgba)
// @param has_transparency indicates if the texture will have transparency
// @param is_writeable indicates if the texture is writeable
// @param internal_data a pointer to the internal data to be set on the texture
// @param register_texture indicates if the texture should be registered with the system
// @return a pointer to the wrapped texture
texture* texture_system_wrap_internal(const char* name, u32 width, u32 height, u8 channel_count, b8 has_transparency, b8 is_writeable, b8 register_texture, void* internal_data);

// @brief sets the internal data of a texture. useful for replacing internal data from within the renderer  for wrapped
// textures for example
// @param t a pointer to the texture to be updated
// @param internal_data a pointer to the internal data to be set on the texture
// @return true on success otherwise false
b8 texture_system_set_internal(texture* t, void* internal_data);

// @brief sets the internal data of a texture. useful for replacing internal data from within the renderer  for wrapped
// textures for example
// @param t a pointer to the texture to be resized
// @param width the new width in pixels
// @param height the new height in pixels
// @param regenerate_internal_data indicates if the internal data should be regenerated
// @return true on success otherwise false
b8 texture_system_resize(texture* t, u32 width, u32 height, b8 regenerate_internal_data);

// @brief writes the given data to the provided texture. may only be used on writeable textures
// @param t a pointer to the texture to be written to
// @param offset the offset in bytes from the beginning of the data to be written
// @param size the number of bytes to be written
// @param data a pointer to the data to be written.
// @return true on success otherwise false
b8 texture_system_write_data(texture* t, u32 offset, u32 size, void* data);

// get a pointer to the default texture
texture* texture_system_get_default_texture();

// @brief gets a pointer to the default diffuse texture. no reference counting is done for default textures
texture* texture_system_get_default_diffuse_texture();

// @brief gets a pointer to the default specular texture. no reference counting is done for default textures
texture* texture_system_get_default_specular_texture();

// @brief gets a pointer to the default normal texture. no reference counting is done for default textures
texture* texture_system_get_default_normal_texture();
