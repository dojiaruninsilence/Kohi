#pragma once

#include "defines.h"
#include "core/asserts.h"
#include "renderer/renderer_types.inl"
#include "containers/freelist.h"

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
    // @brief the amount of memory required for the freelist
    u64 freelist_memormy_requirement;
    // @brief the memory block used by internal free list
    void* freelist_block;
    // @brief a freelist to track allocations
    freelist buffer_freelist;
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
    b8 supports_device_local_host_visible;

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
    vec4 render_area;     // store info for the render area for the renderpass
    vec4 clear_colour;    // store the info for the clear color for the renderpass

    f32 depth;    // store the info for the renderpass depth
    u32 stencil;  // need to look up, but has something to do with depth

    u8 clear_flags;  // flags to describe how the renderpass will handle clearing the screen
    b8 has_prev_pass;
    b8 has_next_pass;

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

    // framebuffers used for onscreen rendering
    VkFramebuffer framebuffers[3];  // an array of framebuffers
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

#define MATERIAL_SHADER_STAGE_COUNT 2  // set the object shader count to 2 for now - these are going to be the vertex and fragment shaders for now

typedef struct vulkan_descriptor_state {
    // one per frame
    u32 generations[3];  // has this descritor been updated or does it need to be updated
    u32 ids[3];          // texture ids
} vulkan_descriptor_state;

#define VULKAN_MATERIAL_SHADER_DESCRIPTOR_COUNT 2  // the amount per object
#define VULKAN_MATERIAL_SHADER_SAMPLER_COUNT 1     // amount of samplers per object

typedef struct vulkan_material_shader_instance_state {  // this struct is one per object, every object has one
    // per frame
    VkDescriptorSet descriptor_sets[3];

    // per descriptor
    vulkan_descriptor_state descriptor_states[VULKAN_MATERIAL_SHADER_DESCRIPTOR_COUNT];
} vulkan_material_shader_instance_state;

// max number of material instances
// TODO: make configurable
#define VULKAN_MAX_MATERIAL_COUNT 1024

// max number of simultaneos uploaded geometries
// TODO: make configurable
#define VULKAN_MAX_GEOMETRY_COUNT 4096

// @brief Internal Buffer data for geometry
typedef struct vulkan_geometry_data {
    u32 id;                    // matches the internal id in the geometry structure
    u32 generation;            // keep track here too
    u32 vertex_count;          // total number of vertices in the geometry
    u32 vertex_element_size;   // size of each vertex times the count, size in bytes
    u64 vertex_buffer_offset;  // how far into the geometry buffer from the beginning
    u32 index_count;           // same but for indices
    u32 index_element_size;
    u64 index_buffer_offset;
} vulkan_geometry_data;

// nvidia has a deal where these have to be 256 bytes perfectly, so its set up like this
// where we are going to hold the data for the global uniforms
typedef struct vulkan_material_shader_global_ubo {
    mat4 projection;   // store projection matrices - 64bytes
    mat4 view;         // store view matrices       - 64bytes
    mat4 m_reserved0;  // 64 bytes, reserved for future use
    mat4 m_reserved1;  // 64 bytes, reserved for future use
} vulkan_material_shader_global_ubo;

// this name will change, but for now - like the global ubo, but this one is for each object - so potentially updating every frame for every object
typedef struct vulkan_material_shader_instance_ubo {
    vec4 diffuse_color;  // 16 bytes
    vec4 v_reserved0;    // 16 bytes, reserved for future use
    vec4 v_reserved1;    // 16 bytes, reserved for future use
    vec4 v_reserved2;    // 16 bytes, reserved for future use
    mat4 m_reserved0;    // 64 bytes, reserved for future use // added from future changes to limp along Shader system feature #38  https://github.com/travisvroman/kohi/pull/38
    mat4 m_reserved1;    // 64 bytes, reserved for future use
    mat4 m_reserved2;    // 64 bytes, reserved for future use
} vulkan_material_shader_instance_ubo;

typedef struct vulkan_material_shader {
    // vertex, fragment, ect
    vulkan_shader_stage stages[MATERIAL_SHADER_STAGE_COUNT];  // hold an array of shader stages, using the count of object shader stages this struct will hold

    // descriptor stuffs
    VkDescriptorPool global_descriptor_pool;             // descriptors are like command buffers, in that that come from and return too a pool
    VkDescriptorSetLayout global_descriptor_set_layout;  // defines the layout of a descriptor set

    // one descriptor set per frame - max 3 for triple-buffering
    VkDescriptorSet global_descriptor_sets[3];
    b8 descriptor_updated[3];

    // global uniform object
    vulkan_material_shader_global_ubo global_ubo;  // store global object stuffs like view, and projection matrices

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

    texture_use sampler_uses[VULKAN_MATERIAL_SHADER_SAMPLER_COUNT];

    // TODO: make dynamic
    vulkan_material_shader_instance_state instance_states[VULKAN_MAX_MATERIAL_COUNT];

    // pipeline
    vulkan_pipeline pipeline;  // hold the vulkan pipeline struct

} vulkan_material_shader;

#define UI_SHADER_STAGE_COUNT 2
#define VULKAN_UI_SHADER_DESCRIPTOR_COUNT 2
#define VULKAN_UI_SHADER_SAMPLER_COUNT 1

// max number of ui control instances
// TODO: make configurable
#define VULKAN_MAX_UI_COUNT 1024

// store the state for the ui shader
typedef struct vulkan_ui_shader_instance_state {
    // per frame
    VkDescriptorSet descriptor_sets[3];

    // per descriptor
    vulkan_descriptor_state descriptor_states[VULKAN_UI_SHADER_DESCRIPTOR_COUNT];
} vulkan_ui_shader_instance_state;

// @brief vulkan specific uniform buffer object for the ui shader.
typedef struct vulkan_ui_shader_global_ubo {
    mat4 projection;   // store projection matrices - 64bytes
    mat4 view;         // store view matrices       - 64bytes
    mat4 m_reserved0;  // 64 bytes, reserved for future use
    mat4 m_reserved1;  // 64 bytes, reserved for future use
} vulkan_ui_shader_global_ubo;

// @brief vulkan specific ui material instance uniform buffer object for the ui shader
typedef struct vulkan_ui_shader_instance_ubo {
    vec4 diffuse_color;  // 16 bytes
    vec4 v_reserved0;    // 16 bytes, reserved for future use
    vec4 v_reserved1;    // 16 bytes, reserved for future use
    vec4 v_reserved2;    // 16 bytes, reserved for future use
    mat4 m_reserved0;    // 64 bytes, reserved for future use // added from future changes to limp along Shader system feature #38  https://github.com/travisvroman/kohi/pull/38
    mat4 m_reserved1;    // 64 bytes, reserved for future use
    mat4 m_reserved2;    // 64 bytes, reserved for future use
} vulkan_ui_shader_instance_ubo;

// store the ui shader itself
typedef struct vulkan_ui_shader {
    // vertex, fragment
    vulkan_shader_stage stages[UI_SHADER_STAGE_COUNT];

    // descriptor stuffs
    VkDescriptorPool global_descriptor_pool;             // descriptors are like command buffers, in that that come from and return too a pool
    VkDescriptorSetLayout global_descriptor_set_layout;  // defines the layout of a descriptor set

    // one descriptor set per frame - max 3 for triple-buffering
    VkDescriptorSet global_descriptor_sets[3];
    b8 descriptor_updated[3];

    // global uniform object
    vulkan_ui_shader_global_ubo global_ubo;  // store global object stuffs like view, and projection matrices

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

    texture_use sampler_uses[VULKAN_UI_SHADER_SAMPLER_COUNT];

    // TODO: make dynamic
    vulkan_ui_shader_instance_state instance_states[VULKAN_MAX_UI_COUNT];

    // pipeline
    vulkan_pipeline pipeline;  // hold the vulkan pipeline struct
} vulkan_ui_shader;

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
    vulkan_renderpass ui_renderpass;    // store the ui renderpass

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

    u32 in_flight_fence_count;    // need to keep track of how many fences are in flight
    VkFence in_flight_fences[2];  // the array of fences in flight

    // holds pointers to fences which exist and are owned elsewhere, one per frame
    VkFence* images_in_flight[3];  // possibly they are in in flight fences at the moment, keeps track of them

    u32 image_index;    // to keep track of images in the swap chain - index of the image that we are currently using
    u32 current_frame;  // keep track of the frames

    b8 recreating_swapchain;  // a state that needs to be tracked in the render loop

    vulkan_material_shader material_shader;  // where we store the object shader infos
    vulkan_ui_shader ui_shader;              // where the ui shader is stored

    // TODO: make dynamic
    vulkan_geometry_data geometries[VULKAN_MAX_GEOMETRY_COUNT];

    // framebuffers used for world rendering, one per frame
    VkFramebuffer world_framebuffers[3];

    i32 (*find_memory_index)(u32 type_filter, u32 property_flags);  // a fuction pointer that takes in a type filter and the property flags - returns a 32 bit int

} vulkan_context;

// where we can store the vulkan specific internal texture data
typedef struct vulkan_texture_data {
    vulkan_image image;
    VkSampler sampler;
} vulkan_texture_data;