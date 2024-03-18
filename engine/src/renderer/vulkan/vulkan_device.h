#pragma once

#include "vulkan_types.inl"

b8 vulkan_device_create(vulkan_context* context);  // context to create the device for

void vulkan_device_destroy(vulkan_context* context);  // context that contains the device to destroy

void vulkan_device_query_swapchain_support(
    VkPhysicalDevice physical_device,                  // takes in the vulkan physical device
    VkSurfaceKHR surface,                              // takes in the vulkan surface
    vulkan_swapchain_support_info* out_support_info);  // the support info to be queried

b8 vulkan_device_detect_depth_format(vulkan_device* device);  // used to get the depth format, pass in a vulkan device -- detects the image format that is required by our depth buffer - returns true if it is the correct format
