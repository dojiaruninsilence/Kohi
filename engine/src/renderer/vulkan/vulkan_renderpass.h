#pragma once

#include "vulkan_types.inl"

// so multiple renderpasses can follow eachother and keep the data from previous passes properly
typedef enum renderpass_clear_flag {
    RENDERPASS_CLEAR_NONE_FLAG = 0x0,
    RENDERPASS_CLEAR_COLOUR_BUFFER_FLAG = 0x1,
    RENDERPASS_CLEAR_DEPTH_BUFFER_FLAG = 0x2,
    RENDERPASS_CLEAR_STENCIL_BUFFER_FLAG = 0x4,
} renderpass_clear_flag;

// create a vulkan renderpass
void vulkan_renderpass_create(
    vulkan_context* context,            // pass in a pointer to vulkan context
    vulkan_renderpass* out_renderpass,  // pass in a pointer to the vulkan renderpass being created
    vec4 render_area,                   // pass in an x any y value, and a width and a height - this will be the render area
    vec4 clear_colour,                  // pass in rgba values - this will be the clear color
    f32 depth,                          // pass in a depth
    u32 stencil,                        // pass in a stencil
    u8 clear_flags,                     // set flags for the type of clearing the renderpass will perform
    b8 has_prev_pass,
    b8 has_next_pass);

// destroy the vulkan renderpass - pass in a pointer to the vulkan context, and a pointer to the renderpass to destroy
void vulkan_renderpass_destroy(vulkan_context* context, vulkan_renderpass* renderpass);

// begin a render pass
void vulkan_renderpass_begin(
    vulkan_command_buffer* command_buffer,  // pass in a vulkan command buffer
    vulkan_renderpass* renderpass,          // pass in the vulkan render pass to begin
    VkFramebuffer frame_buffer);            // pass in a vulkan frame buffer

// end the vulkan render pass - pass in the vulkan command buffer and the vulkan renderpass to end -- this was the last line that i wrote - video is at 7 33
void vulkan_renderpass_end(vulkan_command_buffer* command_buffer, vulkan_renderpass* renderpass);