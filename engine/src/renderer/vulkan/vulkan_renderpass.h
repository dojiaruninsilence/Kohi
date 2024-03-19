#pragma once

#include "vulkan_types.inl"

// create a vulkan renderpass
void vulkan_renderpass_create(
    vulkan_context* context,            // pass in a pointer to vulkan context
    vulkan_renderpass* out_renderpass,  // pass in a pointer to the vulkan renderpass being created
    f32 x, f32 y, f32 w, f32 h,         // pass in an x any y value, and a width and a height - this will be the render area - this and next line will be vec4s after we build our math library
    f32 r, f32 g, f32 b, f32 a,         // pass in rgba values - this will be the clear color
    f32 depth,                          // pass in a depth
    u32 stencil);                       // pass in a stencil

// destroy the vulkan renderpass - pass in a pointer to the vulkan context, and a pointer to the renderpass to destroy
void vulkan_renderpass_destroy(vulkan_context* context, vulkan_renderpass* renderpass);

// begin a render pass
void vulkan_renderpass_begin(
    vulkan_command_buffer* command_buffer,  // pass in a vulkan command buffer
    vulkan_renderpass* renderpass,          // pass in the vulkan render pass to begin
    VkFramebuffer frame_buffer);            // pass in a vulkan frame buffer

// end the vulkan render pass - pass in the vulkan command buffer and the vulkan renderpass to end -- this was the last line that i wrote - video is at 7 33
void vulkan_renderpass_end(vulkan_command_buffer* command_buffer, vulkan_renderpass* renderpass);