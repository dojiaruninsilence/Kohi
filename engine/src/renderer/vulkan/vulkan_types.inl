#pragma once

#include "defines.h"
#include "core/asserts.h"
#include "renderer/renderer_types.inl"

#include <vulkan/vulkan.h>

// checks the given expression's return value against VK_SUCCESS
#define VK_CHECK(expr)               \
    {                                \
        KASSERT(expr == VK_SUCCESS); \
    }

// where we are storing the info for the vulkan buffers
typedef struct vulkan_buffer {
    u64 total_size;               // total size the buffer is taking
    VkBuffer handle;              // handle to the actuall buffer that holds the data
    VkBufferUsageFlagBits usage;  // hold the usage of the buffer
    b8 is_locked;                 // is the buffer locked
    VkDeviceMemory memory;        // allocate and hold on to device memory
    i32 memory_index;             // where the memory is currently at
    u32 memory_property_flags;    // and the type of memory
} vulkan_buffer;

// where we are storing the swapchain support info
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

    VkCommandPool graphics_command_pool;  // handle to the vulkan graphics command pool

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

// where we store the data for the vulkan framebuffer
typedef struct vulkan_framebuffer {
    VkFramebuffer handle;           // store the handle to the framebuffer
    u32 attachment_count;           // keep a count of the number of attachments
    VkImageView* attachments;       // a pointer to an array of attachments
    vulkan_renderpass* renderpass;  // hold a pointer to the renderpass it is associated with
} vulkan_framebuffer;

// where we hold the data for the vulkan swapchain
typedef struct vulkan_swapchain {
    VkSurfaceFormatKHR image_format;  // format for the images that we render too
    u8 max_frames_in_flight;          // number of frames that can be rendered to
    VkSwapchainKHR handle;            // handle to the vulkan swap chain - is an extension
    u32 image_count;                  // keep a count of the images
    VkImage* images;                  // pointer to an array of the images
    VkImageView* views;               // images arent accessed directly in vulkan instead accessed through views - so every image has an accompanying view

    vulkan_image depth_attachment;  // where to store the info for the depth image

    // framebuffers used for onscreen rendering
    vulkan_framebuffer* framebuffers;  // an array of framebuffers
} vulkan_swapchain;

// an enumeration of all the vulkan command buffer states - these are actually changed within vulkan, but we need to keep track of them for the application
typedef enum vulkan_command_buffer_state {
    COMMAND_BUFFER_STATE_READY,            // ready to begin
    COMMAND_BUFFER_STATE_RECORDING,        // need to come back to - the point where it is able to start issuing commands
    COMMAND_BUFFER_STATE_IN_RENDER_PASS,   // currently issuing commands
    COMMAND_BUFFER_STATE_RECORDING_ENDED,  // not issuing commands anymore
    COMMAND_BUFFER_STATE_SUBMITTED,        // execute all of the commands that are submitted
    COMMAND_BUFFER_STATE_NOT_ALLOCATED     // the default state - when it first starts
} vulkan_command_buffer_state;

// where ww will be storing the info for the vulkan command buffer
typedef struct vulkan_command_buffer {
    VkCommandBuffer handle;  // store the handle to the command buffer

    // command buffer state
    vulkan_command_buffer_state state;  // keep track of the state of the command buffer
} vulkan_command_buffer;

// where we store the info for fences
typedef struct vulkan_fence {
    VkFence handle;  // holds a handle to the fence
    b8 is_signaled;  // and a bool on whether it is signaled or not - signaled because an operation has completed
} vulkan_fence;

// where we hold the data for creating shader stages
typedef struct vulkan_shader_stage {
    VkShaderModuleCreateInfo create_info;                      // vulkan info struct for creating shader modules
    VkShaderModule handle;                                     // handle to the shader module thats created
    VkPipelineShaderStageCreateInfo shader_stage_create_info;  // vulkan info struct for creating shader stages
} vulkan_shader_stage;

// where we hold the data for a vulkan pipeline
typedef struct vulkan_pipeline {
    VkPipeline handle;                 // store the handle to the pipeline
    VkPipelineLayout pipeline_layout;  // store the piplines layout
} vulkan_pipeline;

#define OBJECT_SHADER_STAGE_COUNT 2  // set the object shader count to 2 for now - these are going to be the vertex and fragment shaders for now

typedef struct vulkan_descriptor_state {
    // one per frame
    u32 generations[3];  // has this descritor been updated or does it need to be updated
} vulkan_descriptor_state;

#define VULKAN_OBJECT_SHADER_DESCRIPTOR_COUNT 2     // the amount per object
typedef struct vulkan_object_shader_object_state {  // this struct is one per object, every object has one
    // per frame
    VkDescriptorSet descriptor_sets[3];

    // per descriptor
    vulkan_descriptor_state descriptor_states[VULKAN_OBJECT_SHADER_DESCRIPTOR_COUNT];
} vulkan_object_shader_object_state;

// max number of objects - this is a temporary thing
#define VULKAN_OBJECT_MAX_OBJECT_COUNT 1024

typedef struct vulkan_object_shader {
    // vertex, fragment, ect
    vulkan_shader_stage stages[OBJECT_SHADER_STAGE_COUNT];  // hold an array of shader stages, using the count of object shader stages this struct will hold

    // descriptor stuffs
    VkDescriptorPool global_descriptor_pool;             // descriptors are like command buffers, in that that come from and return too a pool
    VkDescriptorSetLayout global_descriptor_set_layout;  // defines the layout of a descriptor set

    // one descriptor set per frame - max 3 for triple-buffering
    VkDescriptorSet global_descriptor_sets[3];

    // global uniform object
    global_uniform_object global_ubo;  // store global object stuffs like view, and projection matrices

    // global uniform buffer
    vulkan_buffer global_uniform_buffer;

    // object level stuffs
    // descriptor stuffs
    VkDescriptorPool object_descriptor_pool;  // same as for global ones but for objects instead
    VkDescriptorSetLayout object_descriptor_set_layout;
    // object uniform buffers - large buffer to handle all objects
    vulkan_buffer object_uniform_buffer;
    // TODO: manage a free list of some kind here instead
    u32 object_uniform_buffer_index;

    // TODO: make dynamic
    vulkan_object_shader_object_state object_states[VULKAN_OBJECT_MAX_OBJECT_COUNT];

    // pipeline
    vulkan_pipeline pipeline;  // hold the vulkan pipeline struct

} vulkan_object_shader;

// this is where we hold all of our static data for this renderer
typedef struct vulkan_context {
    // store the delta time
    f32 frame_delta_time;

    // the framebuffer's current width
    u32 framebuffer_width;

    // the framebuffer's current height
    u32 framebuffer_height;

    // if the next two values are out of sync we will know that a resize has happened
    // current generation of framebuffer size. if it does not match frame_buffer_size_last_genertation, a new one should be generated
    u64 framebuffer_size_generation;  // will be defaulted to zero

    // the generation of the framebuffer when it was last created. set to frame_buffer_size_generation when updated
    u64 framebuffer_size_last_generation;  // keep track of the last time the frambuffer size had been changed

    VkInstance instance;               // vulkan instance, part of the vulkan library, all vulkan stuff is going to be preppended with 'VK' -- handle of the instance
    VkAllocationCallbacks* allocator;  // vulkan memory allocator
    VkSurfaceKHR surface;              // vulkan needs a surface to render to. add it to the context, which will come from the platform layer

#if defined(_DEBUG)                            // if in debug mode
    VkDebugUtilsMessengerEXT debug_messenger;  // vk debug messanger extension - or a handle to it
#endif

    vulkan_device device;  // include the device info in renderer data

    vulkan_swapchain swapchain;         // vulkan swapchain - controls images to be rendered to and presented, hold the images
    vulkan_renderpass main_renderpass;  // store the main vulkan render pass

    // buffers stuffs
    vulkan_buffer object_vertex_buffer;  // store vertex buffers
    vulkan_buffer object_index_buffer;   // store index buffers

    // darray -- we are going to have a whole array of command buffers
    vulkan_command_buffer* graphics_command_buffers;  // an array to store all of the graphics queue command buffers

    // syncronization objects
    // darray
    VkSemaphore* image_available_semaphores;  // when an image is available for rendering, these semaphores are triggered

    // darray
    VkSemaphore* queue_complete_semaphores;  // triggered when a queue has been run and the image is ready to be presented

    u32 in_flight_fence_count;       // need to keep track of how many fences are in flight
    vulkan_fence* in_flight_fences;  // the array of fences in flight

    // holds pointers to fences which exist and are owned elsewhere
    vulkan_fence** images_in_flight;  // possibly they are in in flight fences at the moment, keeps track of them

    u32 image_index;    // to keep track of images in the swap chain - index of the image that we are currently using
    u32 current_frame;  // keep track of the frames

    b8 recreating_swapchain;  // a state that needs to be tracked in the render loop

    vulkan_object_shader object_shader;  // where we store the object shader infos

    u64 geometry_vertex_offset;  // offset to keep track of everytime that we load data into the buffer
    u64 geometry_index_offset;   // offset to keep track of everytime that we load data into the buffer

    i32 (*find_memory_index)(u32 type_filter, u32 property_flags);  // a fuction pointer that takes in a type filter and the property flags - returns a 32 bit int

} vulkan_context;

// where we can store the vulkan specific internal texture data
typedef struct vulkan_texture_data {
    vulkan_image image;
    VkSampler sampler;
} vulkan_texture_data;