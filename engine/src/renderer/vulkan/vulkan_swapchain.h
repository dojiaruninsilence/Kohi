#pragma once

#include "vulkan_types.inl"

// create a vulkan swapchain
void vulkan_swapchain_create(
    vulkan_context* context,           // pass in a vulkan context
    u32 width,                         // pass in a width
    u32 height,                        // pass in a height
    vulkan_swapchain* out_swapchain);  // pass out a swap chain struct

// in place of update - a swapchain is unchangeable - so in te event it would need to be uopdated it instead needs to be destroyed and recreated - but not all of the resources need to be
void vulkan_swapchain_recreate(
    vulkan_context* context,       // pass in a vulkan context
    u32 width,                     // pass in a width
    u32 height,                    // pass in a height
    vulkan_swapchain* swapchain);  // pass in a swap chain struct

// destroy a vulkan swapchain
void vulkan_swapchain_destroy(
    vulkan_context* context,       // pass in the context
    vulkan_swapchain* swapchain);  // pass in the swapchain to be destroyed

// gives us the index of next image in the swapchain to be used - then eventually to be returned to the queue in the right place
b8 vulkan_swapchain_acquire_next_image_index(
    vulkan_context* context,                // pass in the context
    vulkan_swapchain* swapchain,            // pass in the swapchain struct
    u64 timeout_ns,                         // pass in a timeout value, in nano secs?
    VkSemaphore image_available_semaphore,  // pass in a semaphore - which is a way to sync up gpu operations with other gpu operations
    VkFence fence,                          // pass in a fence - which is used to sync up operations between the application and the gpu
    u32* out_image_index);                  // pointer to the out img index we wanted

void vulkan_swapchain_present(
    vulkan_context* context,      // pass in a pointer to the context
    vulkan_swapchain* swapchain,  // pass in a pointer to the swapchain struct
    VkQueue graphics_queue,       // pass in the handle to the graphics queue
    VkQueue present_queue,        // pass in the handle to the present queue
    VkSemaphore render_complete_semaphore,
    u32 present_image_index);  // pass in the present image index