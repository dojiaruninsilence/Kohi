#pragma once

#include "defines.h"

#include <vulkan/vulkan.h>

// this is where we hold all of our static data for this renderer
typedef struct vulkan_context {
    VkInstance instance;  // vulkan instance, part of the vulkan library, all vulkan stuff is going to be preppended with 'VK' -- handle of the instance
    VkAllocationCallbacks* allocator;
} vulkan_context;
