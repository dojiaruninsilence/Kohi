#pragma once

#include "vulkan_types.inl"

void vulkan_image_create(
    vulkan_context* context,               // pass in the vulkan context
    VkImageType image_type,                // pass in the image type
    u32 width,                             // pass in a width
    u32 height,                            // pass in a height
    VkFormat format,                       // pass in a vulkan format
    VkImageTiling tiling,                  //
    VkImageUsageFlags usage,               // how the image is going to be used
    VkMemoryPropertyFlags memory_flags,    // if there any that we need to actually set
    b32 create_view,                       // whether or not we want to create a view with it
    VkImageAspectFlags view_aspect_flags,  // any aspect flags that we may need may not
    vulkan_image* out_image);              // pointer to a vulcan image struct

void vulkan_image_view_create(
    vulkan_context* context,           // pass in a pointer to the vulkan context
    VkFormat format,                   // pass in the vulkan format
    vulkan_image* image,               // pass in the vulcan image the view is being created for
    VkImageAspectFlags aspect_flags);  // any aspect flags that we may or may not need

// transitions the provided image from old_layout to new_layout- convert from an image the disk understands to an image the gpu understands
// pass it pointers to the context, a command buffer, and a vulkan image, also pass it the format, and the old and new layouts
void vulkan_image_transition_layout(
    vulkan_context* context,
    vulkan_command_buffer* command_buffer,
    vulkan_image* image,
    VkFormat format,
    VkImageLayout old_layout,
    VkImageLayout new_layout);

// copies data in buffer to provided image
// @param context the vulkan context
// @param image the image to copy the buffer's data to
// @param the buffer whose data will be copied
void vulkan_image_copy_from_buffer(
    vulkan_context* context,
    vulkan_image* image,
    VkBuffer buffer,
    vulkan_command_buffer* command_buffer);

// destroy a vulkan image, pass in a pointer to the vulkan context, and a pointer to the image being destroyed
void vulkan_image_destroy(vulkan_context* context, vulkan_image* image);