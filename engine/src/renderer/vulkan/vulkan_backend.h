#pragma once

#include "renderer/renderer_backend.h"

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
void vulkan_renderer_update_global_state(mat4 projection, mat4 view, vec3 view_position, vec4 ambient_colour, i32 mode);

// everything that needs to be done to end a frame
b8 vulkan_renderer_backend_end_frame(renderer_backend* backend, f32 delta_time);

// update an object using push constants, input a model to upload
void vulkan_backend_update_object(mat4 model);
