#pragma once

#include "vulkan_types.inl"

// create a vulkan fence
void vulkan_fence_create(
    vulkan_context* context,   // pass in a pointer to the vulkan context
    b8 create_signaled,        // if true will be created in a signaled state
    vulkan_fence* out_fence);  // and a pointer to the fence being created

// destroy a vulkan fence - pass in a pointer to the context and the fence to be destroyed
void vulkan_fence_destroy(vulkan_context* context, vulkan_fence* fence);

// function to tell a fence to wait, takes in a pointer to the vulkan context, and a pointer to the fence to wait, and the time it should wait in nano seconds
b8 vulkan_fence_wait(vulkan_context* context, vulkan_fence* fence, u64 timeout_ns);

// reset a vulkan fence - takes in a pointer to the vulkan context, and a pointer to the fence being reset
void vulkan_fence_reset(vulkan_context* context, vulkan_fence* fence);