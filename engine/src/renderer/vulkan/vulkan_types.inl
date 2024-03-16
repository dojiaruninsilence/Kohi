#pragma once

#include "defines.h"
#include "core/asserts.h"

#include <vulkan/vulkan.h>

// checks the given expression's return value against VK_SUCCESS
#define VK_CHECK(expr)               \
    {                                \
        KASSERT(expr == VK_SUCCESS); \
    }

// this is where we hold all of our static data for this renderer
typedef struct vulkan_context {
    VkInstance instance;  // vulkan instance, part of the vulkan library, all vulkan stuff is going to be preppended with 'VK' -- handle of the instance
    VkAllocationCallbacks* allocator;
#if defined(_DEBUG)                            // if in debug mode
    VkDebugUtilsMessengerEXT debug_messenger;  // vk debug messanger extension - or a handle to it
#endif
} vulkan_context;
