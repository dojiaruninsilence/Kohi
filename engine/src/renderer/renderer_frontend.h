#pragma once

#include "renderer_types.inl"

struct shader;
struct shader_uniform;

// initialize the renderer subsystem, - always call twice - on first pass pass in the memory requirement to get the memory required, and zero for the state
// on the second pass - pass in the state as well as the memory rewuirement and actually initialize the subsystem, also pass in a pointer to the application name
b8 renderer_system_initialize(u64* memory_requirement, void* state, const char* application_name);  // takes in pointers to the application name and the platform state
void renderer_system_shutdown(void* state);

void renderer_on_resized(u16 width, u16 height);  // may change the structure of this later, keeping it simple for now

// called once per frame to take off all the rendering
// a renderer packet is a packet full of information prepared ahead of time
// contains all the info that the renderer needs to know to draw that frame
b8 renderer_draw_frame(render_packet* packet);

// create a texture pass in a pointer to the pixels in a u8 array, that is 8 bits per pixel and an address for the texture struct
void renderer_texture_create(const u8* pixels, struct texture* texture);

// destroy a texture
void renderer_texture_destroy(struct texture* texture);

// @brief creates a new writeable texture with no data written to it
// @param t a pointer to the texture to hold the resources
void renderer_texture_create_writeable(texture* t);

// @brief resizes a texture. there is no check at this level to see if the texture is writeable. internal resources are
// destroyed and recreated at the new resolution. data is lost and would need to be reloaded
// @param t a pointer to the texture to be resized
// @param new_width the width in pixels
// @param new_height the height in pixels
void renderer_texture_resize(texture* t, u32 new_width, u32 new_height);

// @brief writes the given data to the provided texture
// @param t a pointer to the texture to be written to NOTE: must be a writeable texture
// @param offset the offset in bytes from the beginning of the data to be written
// @param size the number of bytes to be written
// @param pixels the raw image data to be written
void renderer_texture_write_data(texture* t, u32 offset, u32 size, const u8* pixels);

// geometry
b8 renderer_create_geometry(geometry* geometry, u32 vertex_size, u32 vertex_count, const void* vertices, u32 index_size, u32 index_count, const void* indices);
void renderer_destroy_geometry(geometry* geometry);

// @brief draws the given geometry. should only be called inside a renderpass, within a frame
// @param data the render data of the geometry to be drawn
void renderer_draw_geometry(geometry_render_data* data);

// @brief begins the given renderpass
// @param pass a pointer to the renderpass to begin
// @param target a pointer to the render target to be used
// @return true on success, otherwise false
b8 renderer_renderpass_begin(renderpass* pass, render_target* target);

// @brief ends the given renderpass
// @param pass a pointer to the render target to be used
// @return true on success, otherwise false
b8 renderer_renderpass_end(renderpass* pass);

// @brief obtains a pointer to the renderpass with the given name.
// @param name the name of the renderpass whose identifier to obtain.
// @return a pointer to the render pass if found, otherwise 0
renderpass* renderer_renderpass_get(const char* name);

// @brief creates internal shader resources using the provided parameters.
// @param s a pointer to the shader.
// @param config a constant pointer to the shader config
// @param pass a pointer to the renderpass to be associated with the shader.
// @param stage_count the total number of stages.
// @param stage_filenames an array of shader stage filenames to be loaded. Should align with stages array.
// @param stages a array of shader_stages indicating what render stages (vertex, fragment, etc.) used in this shader.
// @return b8 true on success, otherwise false.
b8 renderer_shader_create(struct shader* s, const shader_config* config, renderpass* pass, u8 stage_count, const char** stage_filenames, shader_stage* stages);

// @brief destroys the given shader and releases any resources held by it.
// @param s a pointer to the shader to be destroyed.
void renderer_shader_destroy(struct shader* s);

// @brief initializes a configured shader. will be automatically destroyed if this step fails.
// must be done after vulkan_shader_create().
// @param s a pointer to the shader to be initialized.
// @return true on success, otherwise false.
b8 renderer_shader_initialize(struct shader* s);

// @brief uses the given shader, activating it for updates to attributes, uniforms and such,
// and for use in draw calls.
// @param s a pointer to the shader to be used.
// @return true on success, otherwise false.
b8 renderer_shader_use(struct shader* s);

// @brief binds global resources for use and updating.
// @param s a pointer to the shader whose globals are to be bound.
// @return true on success, otherwise false.
b8 renderer_shader_bind_globals(struct shader* s);

// @brief binds instance resources for use and updating.
// @param s a pointer to the shader whose instance resources are to be bound.
// @param instance_id the identifier of the instance to be bound.
// @return true on success, otherwise false.
b8 renderer_shader_bind_instance(struct shader* s, u32 instance_id);

// @brief applies global data to the uniform buffer.
// @param s a pointer to the shader to apply the global data for.
// @return true on success, otherwise false.
b8 renderer_shader_apply_globals(struct shader* s);

// @brief applies data for the currently bound instance.
// @param s a pointer to the shader to apply the instance data for.
// @param needs_update indicates if the shader uniforms need to be updated or just bound
// @return true on success, otherwise false.
b8 renderer_shader_apply_instance(struct shader* s, b8 needs_update);

// @brief acquires internal instance-level resources and provides an instance id.
// @param s a pointer to the shader to acquire resources from.
// @param maps an array of texture map pointers. must be one per texture in the instance
// @param out_instance_id a pointer to hold the new instance identifier.
// @return true on success, otherwise false.
b8 renderer_shader_acquire_instance_resources(struct shader* s, texture_map** maps, u32* out_instance_id);

// @brief releases internal instance-level resources for the given instance id.
// @param s a pointer to the shader to release resources from.
// @param instance_id the instance identifier whose resources are to be released.
// @return true on success, otherwise false.
b8 renderer_shader_release_instance_resources(struct shader* s, u32 instance_id);

// @brief sets the uniform of the given shader to the provided value.
// @param s a ponter to the shader.
// @param uniform a constant pointer to the uniform.
// @param value a pointer to the value to be set.
// @return b8 true on success, otherwise false.
b8 renderer_set_uniform(struct shader* s, struct shader_uniform* uniform, const void* value);

// @brief acquires internal resources for the given texture map
// @param map a pointer to the texture map to obtain resources for
// @return true on success, otherwise false
b8 renderer_texture_map_acquire_resources(struct texture_map* map);

// @brief releases internal resources for the given texture map
// @param map a pointer to the texture map to release from
void renderer_texture_map_release_resources(struct texture_map* map);

// @brief creates a new render target using the provided data
// @param attachment_count the number of attachements (texture pointers)
// @param attachments an array of attachments (texture pointers)
// @param renderpass a pointer to the renderpass the render target is associated with
// @param width the width of the render target in pixels
// @param height the height of the render target in pixels
// @param out_target a pointer to hold the newly created render targets
void renderer_render_target_create(u8 attachment_count, texture** attachments, renderpass* pass, u32 width, u32 height, render_target* out_target);

// @brief destroys the provided render target
// @param target a pointer to the render target to be destroyed
// @param free_internal_memory indicates if internal memory should be freed

void renderer_render_target_destroy(render_target* target, b8 free_internal_memory);

// @brief creates a new renderpass
// @param out_renderpass a pointer to the generic renderpass
// @param depth the depth clear amount
// @param stencil the sencil clear value
// @param clear_flags the combined clear flags indicating what kind of clear should take place
// @param has_prev_pass indicates if there is a previous renderpass
// @param has_next_pass indicates if there is a next renderpass
void renderer_renderpass_create(renderpass* out_renderpass, f32 depth, u32 stencil, b8 has_prev_pass, b8 has_next_pass);

// @brief destroys the given renderpass
// @param pass a pointer to the renderpass to be destroyed
void renderer_renderpass_destroy(renderpass* pass);