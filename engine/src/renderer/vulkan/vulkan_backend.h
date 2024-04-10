#pragma once

#include "renderer/renderer_backend.h"
#include "resources/resource_types.h"

// these are the vulkan specific functions, for the pointer functions in the renderer_types.inl

// in itialize the vulkan renderer backend, pass in the pointer to the backend, and a poiter to the application name
b8 vulkan_renderer_backend_initialize(renderer_backend* backend, const char* application_name);

// shut down the vulkan renderer backend, and all the attached sub systems
void vulkan_renderer_backend_shutdown(renderer_backend* backend);

// whenever the screen is resized, pass in the backend, and the new size
void vulkan_renderer_backend_on_resized(renderer_backend* backend, u16 width, u16 height);

// everything that needs to be done to begin a frame
b8 vulkan_renderer_backend_begin_frame(renderer_backend* backend, f32 delta_time);

// update the global state, this will probably control camera movements and such, pass in a view, and projection matrices, the view position, ambient color and the mode
void vulkan_renderer_update_global_world_state(mat4 projection, mat4 view, vec3 view_position, vec4 ambient_colour, i32 mode);

// update the global ui state
void vulkan_renderer_update_global_ui_state(mat4 projection, mat4 view, i32 mode);

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

// create a material
b8 vulkan_renderer_create_material(struct material* material);

// destroy a material
void vulkan_renderer_destroy_material(struct material* material);

// create geometry
b8 vulkan_renderer_create_geometry(geometry* geometry, u32 vertex_count, const vertex_3d* vertices, u32 index_count, const u32* indices);

// destroy geometry
void vulkan_renderer_destroy_geometry(geometry* geometry);
