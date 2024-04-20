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

// deprecated HACK: this should not be exposed outside the engine
// @param view the view matrix to be set
// @param view_position the view position to be set
KAPI void renderer_set_view(mat4 view, vec3 view_position);

// create a texture pass in a pointer to the pixels in a u8 array, that is 8 bits per pixel and an address for the texture struct
void renderer_create_texture(const u8* pixels, struct texture* texture);

// destroy a texture
void renderer_destroy_texture(struct texture* texture);

// geometry
b8 renderer_create_geometry(geometry* geometry, u32 vertex_size, u32 vertex_count, const void* vertices, u32 index_size, u32 index_count, const void* indices);
void renderer_destroy_geometry(geometry* geometry);

// @brief obtains the identifier of the renderpass with the given name.
// @param name the name of the renderpass whose identifier to obtain.
// @param out_renderpass_id a pointer to hold the renderpass id.
// @return true if found, otherwise false.
b8 renderer_renderpass_id(const char* name, u8* out_renderpass_id);

// @brief creates internal shader resources using the provided parameters.
// @param s a pointer to the shader.
// @param renderpass_id the identifier of the renderpass to be associated with the shader.
// @param stage_count the total number of stages.
// @param stage_filenames an array of shader stage filenames to be loaded. Should align with stages array.
// @param stages a array of shader_stages indicating what render stages (vertex, fragment, etc.) used in this shader.
// @return b8 true on success, otherwise false.
b8 renderer_shader_create(struct shader* s, u8 renderpass_id, u8 stage_count, const char** stage_filenames, shader_stage* stages);

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
// @param out_instance_id a pointer to hold the new instance identifier.
// @return true on success, otherwise false.
b8 renderer_shader_acquire_instance_resources(struct shader* s, u32* out_instance_id);

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