#pragma once

#include "renderer/renderer_backend.h"

// these are the vulkan specific functions, for the pointer functions in the renderer_types.inl

// in itialize the vulkan renderer backend, pass in the pointer to the backend, and a poiter to the application name
b8 vulkan_renderer_backend_initialize(renderer_backend* backend, const char* application_name);
void vulkan_renderer_backend_shutdown(renderer_backend* backend);

void vulkan_renderer_backend_on_resized(renderer_backend* backend, u16 width, u16 height);

b8 vulkan_renderer_backend_begin_frame(renderer_backend* backend, f32 delta_time);
b8 vulkan_renderer_backend_end_frame(renderer_backend* backend, f32 delta_time);
