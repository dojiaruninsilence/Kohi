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

    VkQueue graphics_queue;  // handle to the graphics queue
    VkQueue present_queue;   // handle to the present queue
    VkQueue transfer_queue;  // handle to the transfer queue

    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;
    VkPhysicalDeviceMemoryProperties memory;

    VkFormat depth_format;  // the depth format of the device
} vulkan_device;

// where we are going to store the info for vulkan image
typedef struct vulkan_image {
    VkImage handle;         // handle to the vulkan image
    VkDeviceMemory memory;  // handle to the memory allocated by that image
    VkImageView view;       // the view assosiated with the image
    u32 width;              // stored for convinience
    u32 height;             // stored for convinience
} vulkan_image;

// an enumeration of the different states that the renderpass can be in
typedef enum vulkan_render_pass_state {
    READY,            // ready to begin
    RECORDING,        // need to come back to
    IN_RENDER_PASS,   // in a render pass
    RECORDING_ENDED,  // not in a render pass anymore
    SUBMITTED,        // render pass submitted and ready for execution
    NOT_ALLOCATED     // the default state - when it first starts
} vulkan_render_pass_state;

// where we will hold the data for the vulkan renderpass
typedef struct vulkan_renderpass {
    VkRenderPass handle;  // store the handle to the vulkan render pass
    f32 x, y, w, h;       // store info for the render area for the renderpass
    f32 r, g, b, a;       // store the info for the clear color for the renderpass

    f32 depth;    // store the info for the renderpass depth
    u32 stencil;  // need to look up, but has something to do with depth

    vulkan_render_pass_state state;  // track the state of the vulkan render pass
} vulkan_renderpass;

// where we hold the data for the vulkan swapchain
typedef struct vulkan_swapchain {
    VkSurfaceFormatKHR image_format;  // format for the images that we render too
    u8 max_frames_in_flight;          // number of frames that can be rendered to
    VkSwapchainKHR handle;            // handle to the vulkan swap chain - is an extension
    u32 image_count;                  // keep a count of the images
    VkImage* images;                  // pointer to an array of the images
    VkImageView* views;               // images arent accessed directly in vulkan instead accessed through views - so every image has an accompanying view

    vulkan_image depth_attachment;  // where to store the info for the depth image
} vulkan_swapchain;

// an enumeration of all the vulkan command buffer states
typedef enum vulkan_command_buffer_state {
    COMMAND_BUFFER_STATE_READY,            // ready to begin
    COMMAND_BUFFER_STATE_RECORDING,        // need to come back to
    COMMAND_BUFFER_STATE_IN_RENDER_PASS,   // in a render pass
    COMMAND_BUFFER_STATE_RECORDING_ENDED,  // not in a render pass anymore
    COMMAND_BUFFER_STATE_SUBMITTED,        // render pass submitted and ready for execution
    COMMAND_BUFFER_STATE_NOT_ALLOCATED     // the default state - when it first starts
} vulkan_command_buffer_state;

// where ww will be storing the info for the vulkan command buffer
typedef struct vulkan_command_buffer {
    VkCommandBuffer handle;  // store the handle to the command buffer

    // command buffer state
    vulkan_command_buffer_state state;  // keep track of the state of the command buffer
} vulkan_command_buffer;

// this is where we hold all of our static data for this renderer
typedef struct vulkan_context {
    // the framebuffer's current width
    u32 framebuffer_width;

    // the framebuffer's current height
    u32 framebuffer_height;

    VkInstance instance;               // vulkan instance, part of the vulkan library, all vulkan stuff is going to be preppended with 'VK' -- handle of the instance
    VkAllocationCallbacks* allocator;  // vulkan memory allocator
    VkSurfaceKHR surface;              // vulkan needs a surface to render to. add it to the context, which will come from the platform layer

#if defined(_DEBUG)                            // if in debug mode
    VkDebugUtilsMessengerEXT debug_messenger;  // vk debug messanger extension - or a handle to it
#endif

    vulkan_device device;  // include the device info in renderer data

    vulkan_swapchain swapchain;         // vulkan swapchain - controls images to be rendered to and presented, hold the images
    vulkan_renderpass main_renderpass;  // store the main vulkan render pass

    u32 image_index;    // to keep track of images in the swap chain - index of the image that we are currently using
    u32 current_frame;  // keep track of the frames

    b8 recreating_swapchain;  // a state that needs to be tracked in the render loop

    i32 (*find_memory_index)(u32 type_filter, u32 property_flags);  // a fuction pointer that takes in a type filter and the property flags - returns a 32 bit int

} vulkan_context;
