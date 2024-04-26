#pragma once

#include "vulkan_types.inl"

// @brief creates a new vulkan image
// @param context a pointer to the vulkan context
// @param type the type of texture. provides hints to creation
// @param width the width of the image. for cubemaps, this is for each side of the cube
// @param height the height of the image. for cubemaps, this is for each side of the cube
// @param format the format of the image
// @param tiling the image tiling mode
// @param usage the image useage
// @param memory_flags memory flags for the memory used by the image
// @param creat_view indicates if a view should be created with this image
// @param view_aspect_flags apsect flags to be used when creating the view, if applicable
// @param out_image a pointer to hold the newly created image
void vulkan_image_create(
    vulkan_context* context,
    texture_type type,
    u32 width,
    u32 height,
    VkFormat format,
    VkImageTiling tiling,
    VkImageUsageFlags usage,
    VkMemoryPropertyFlags memory_flags,
    b32 create_view,
    VkImageAspectFlags view_aspect_flags,
    vulkan_image* out_image);

// @brief creates a view for the given image
// @param context a pointer to the vulkan context
// @param type the type of texture. provides hints for creation
// @param format the image format
// @param image a pointer to the image associated with the view
// @param aspect_flags aspect flags to be used when creating the view, if applicable
void vulkan_image_view_create(
    vulkan_context* context,
    texture_type type,
    VkFormat format,
    vulkan_image* image,
    VkImageAspectFlags aspect_flags);

// @brief transitions the provided image form old layout to new layout
// @param context a pointer to the vulkan context
// @param type the type of texture. provides hints for creation
// @param command_buffer a pointer to the command buffer to be used
// @param image a pointer to the image whose layout will be transitioned
// @param format the image format
// @param old_layout the old layout
// @param new_layout the new layout
void vulkan_image_transition_layout(
    vulkan_context* context,
    texture_type type,
    vulkan_command_buffer* command_buffer,
    vulkan_image* image,
    VkFormat format,
    VkImageLayout old_layout,
    VkImageLayout new_layout);

// copies data in buffer to provided image
// @param context the vulkan context
// @param type the type of texture. provides hints for creation
// @param image the image to copy the buffer's data to
// @param the buffer whose data will be copied
void vulkan_image_copy_from_buffer(
    vulkan_context* context,
    texture_type type,
    vulkan_image* image,
    VkBuffer buffer,
    vulkan_command_buffer* command_buffer);

// destroy a vulkan image, pass in a pointer to the vulkan context, and a pointer to the image being destroyed
void vulkan_image_destroy(vulkan_context* context, vulkan_image* image);