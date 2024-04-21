#pragma once

#include "renderer/renderer_backend.h"
#include "resources/resource_types.h"

struct shader;
struct shader_uniform;

// these are the vulkan specific functions, for the pointer functions in the renderer_types.inl

// in itialize the vulkan renderer backend, pass in the pointer to the backend, and a poiter to the application name
b8 vulkan_renderer_backend_initialize(renderer_backend* backend, const char* application_name);

// shut down the vulkan renderer backend, and all the attached sub systems
void vulkan_renderer_backend_shutdown(renderer_backend* backend);

// whenever the screen is resized, pass in the backend, and the new size
void vulkan_renderer_backend_on_resized(renderer_backend* backend, u16 width, u16 height);

// everything that needs to be done to begin a frame
b8 vulkan_renderer_backend_begin_frame(renderer_backend* backend, f32 delta_time);

// everything that needs to be done to end a frame
b8 vulkan_renderer_backend_end_frame(renderer_backend* backend, f32 delta_time);

// begin a render pass
b8 vulkan_renderer_begin_renderpass(struct renderer_backend* backend, u8 renderpass_id);

// end a render pass
b8 vulkan_renderer_end_renderpass(struct renderer_backend* backend, u8 renderpass_id);

// update an object using push constants, input a model to upload
void vulkan_renderer_draw_geometry(geometry_render_data data);

// create a texture, pass in a name, is it realeased automatically, the size, how many channels it hase,
// a pointer to the pixels in a u8 array, that is 8 bits per pixel, does it need transparency, and an address for the texture struct
void vulkan_renderer_create_texture(const u8* pixels, texture* texture);

// destroy a texture
void vulkan_renderer_destroy_texture(texture* texture);

// create geometry
b8 vulkan_renderer_create_geometry(geometry* geometry, u32 vertex_size, u32 vertex_count, const void* vertices, u32 index_size, u32 index_count, const void* indices);

// destroy geometry
void vulkan_renderer_destroy_geometry(geometry* geometry);

b8 vulkan_renderer_shader_create(struct shader* shader, u8 renderpass_id, u8 stage_count, const char** stage_filenames, shader_stage* stages);
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
