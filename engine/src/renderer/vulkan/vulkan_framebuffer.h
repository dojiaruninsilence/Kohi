#pragma once

#include "vulkan_types.inl"

// create a vulkan framebuffer
void vulkan_framebuffer_create(
    vulkan_context* context,               // takes in a pointer to a vulkan context
    vulkan_renderpass* renderpass,         // a pointer to a vulkan renderpass
    u32 width,                             // a width
    u32 height,                            // a height
    u32 attachment_count,                  // the number of attachments
    VkImageView* attachments,              // a pointer to the array of attachments' views
    vulkan_framebuffer* out_framebuffer);  // and a pointer to the vulkan framebuffer being created

// destroy a vulkan framebuffer
void vulkan_framebuffer_destroy(vulkan_context* context, vulkan_framebuffer* framebuffer);  // takes in a pointer to the vulkan context, and a pointer to the vulkan framebuffer being destroyed
