#pragma once

#include "renderer/renderer_backend.h"
#include "resources/resource_types.h"

struct shader;
struct shader_uniform;

// these are the vulkan specific functions, for the pointer functions in the renderer_types.inl

// in itialize the vulkan renderer backend, pass in the pointer to the backend, and a poiter to the application name
b8 vulkan_renderer_backend_initialize(renderer_backend* backend, const renderer_backend_config* config, u8* out_window_render_target_count);

// shut down the vulkan renderer backend, and all the attached sub systems
void vulkan_renderer_backend_shutdown(renderer_backend* backend);

// whenever the screen is resized, pass in the backend, and the new size
void vulkan_renderer_backend_on_resized(renderer_backend* backend, u16 width, u16 height);

// everything that needs to be done to begin a frame
b8 vulkan_renderer_backend_begin_frame(renderer_backend* backend, f32 delta_time);

// everything that needs to be done to end a frame
b8 vulkan_renderer_backend_end_frame(renderer_backend* backend, f32 delta_time);
b8 vulkan_renderer_begin_renderpass(struct renderer_backend* backend, renderpass* pass, render_target* target);
b8 vulkan_renderer_end_renderpass(struct renderer_backend* backend, renderpass* pass);
renderpass* vulkan_renderer_renderpass_get(const char* name);

// update an object using push constants, input a model to upload
void vulkan_renderer_draw_geometry(geometry_render_data data);

// create a texture, pass in a name, is it realeased automatically, the size, how many channels it hase,
// a pointer to the pixels in a u8 array, that is 8 bits per pixel, does it need transparency, and an address for the texture struct
void vulkan_renderer_texture_create(const u8* pixels, texture* texture);

// destroy a texture
void vulkan_renderer_texture_destroy(texture* texture);

// @brief creates a new writeable texture with no data written to it.
// @param t a pointer to the texture to hold the resources
void vulkan_renderer_texture_create_writeable(texture* t);

// @brief resizes a texture. there is no check at this level to see if the texture is writeable. internal resources
// are destroyed and recreated at the new resolution. data is lost and would need to be reloaded
// @param t a pointer to the texture to be resized.
// @param new_width the width in pixels
// @param new_height the height in pixels
void vulkan_renderer_texture_resize(texture* t, u32 new_width, u32 new_height);

// @brief writes the given data to the provided texture NOTE: at this level, this can either be writeable
// or non writeable texture because this also handles the initial texture load. the texture system itself
// should be responsible for blocking write requests for non writeable textures
// @param t a pointer to the texture to be written to
// @param offset the offset in bytes from the beginning of the data to be written
// @param size the number of bytes to be written
// @param pixels the raw image data to be written
void vulkan_renderer_texture_write_data(texture* t, u32 offset, u32 size, const u8* pixels);

// create geometry
b8 vulkan_renderer_create_geometry(geometry* geometry, u32 vertex_size, u32 vertex_count, const void* vertices, u32 index_size, u32 index_count, const void* indices);

// destroy geometry
void vulkan_renderer_destroy_geometry(geometry* geometry);

b8 vulkan_renderer_shader_create(struct shader* shader, renderpass* pass, u8 stage_count, const char** stage_filenames, shader_stage* stages);
void vulkan_renderer_shader_destroy(struct shader* s);

b8 vulkan_renderer_shader_initialize(struct shader* shader);
b8 vulkan_renderer_shader_use(struct shader* shader);

b8 vulkan_renderer_shader_bind_globals(struct shader* s);
b8 vulkan_renderer_shader_bind_instance(struct shader* s, u32 instance_id);

b8 vulkan_renderer_shader_apply_globals(struct shader* s);
b8 vulkan_renderer_shader_apply_instance(struct shader* s, b8 needs_update);

b8 vulkan_renderer_shader_acquire_instance_resources(struct shader* s, texture_map** maps, u32* out_instance_id);
b8 vulkan_renderer_shader_release_instance_resources(struct shader* s, u32 instance_id);

b8 vulkan_renderer_set_uniform(struct shader* frontend_shader, struct shader_uniform* uniform, const void* value);

b8 vulkan_renderer_texture_map_acquire_resources(texture_map* map);
void vulkan_renderer_texture_map_release_resources(texture_map* map);

void vulkan_renderpass_create(renderpass* out_renderpass, f32 depth, u32 stencil, b8 has_prev_pass, b8 has_next_pass);
void vulkan_renderpass_destroy(renderpass* pass);

void vulkan_renderer_render_target_create(u8 attachment_count, texture** attachments, renderpass* pass, u32 width, u32 height, render_target* out_target);
void vulkan_renderer_render_target_destroy(render_target* target, b8 free_internal_memory);

texture* vulkan_renderer_window_attachment_get(u8 index);
texture* vulkan_renderer_depth_attachment_get();
u8 vulkan_renderer_window_attachment_index_get();
