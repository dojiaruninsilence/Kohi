#pragma once

#include "defines.h"
#include "core/asserts.h"

#include <vulkan/vulkan.h>

// checks the given expression's return value against VK_SUCCESS
#define VK_CHECK(expr)               \
    {                                \
        KASSERT(expr == VK_SUCCESS); \
    }

typedef struct vulkan_swapchain_support_info {
    VkSurfaceCapabilitiesKHR capabilities;  // features and capabilities on the surface
    u32 format_count;                       // count of image formats used for rendering
    VkSurfaceFormatKHR* formats;            // array of image formats for rendering
    u32 present_mode_count;                 // count of the number of presentation modes
    VkPresentModeKHR* present_modes;        // an array of the presentation modes
} vulkan_swapchain_support_info;

// contains (encapsulates) both the physical and logical device
typedef struct vulkan_device {
    VkPhysicalDevice physical_device;                 // used to build logical device
    VkDevice logical_device;                          // used by the application
    vulkan_swapchain_support_info swapchain_support;  // used to check the device
    i32 graphics_queue_index;
    i32 present_queue_index;
    i32 transfer_queue_index;

    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;
    VkPhysicalDeviceMemoryProperties memory;
} vulkan_device;

// this is where we hold all of our static data for this renderer
typedef struct vulkan_context {
    VkInstance instance;  // vulkan instance, part of the vulkan library, all vulkan stuff is going to be preppended with 'VK' -- handle of the instance
    VkAllocationCallbacks* allocator;
    VkSurfaceKHR surface;  // vulkan needs a surface to render to. add it to the context, which will come from the platform layer

#if defined(_DEBUG)                            // if in debug mode
    VkDebugUtilsMessengerEXT debug_messenger;  // vk debug messanger extension - or a handle to it
#endif

    vulkan_device device;  // include the device info in renderer data
} vulkan_context;
