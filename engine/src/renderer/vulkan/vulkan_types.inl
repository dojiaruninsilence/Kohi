#pragma once

#include "defines.h"
#include "core/asserts.h"
#include "renderer/renderer_types.inl"
#include "containers/freelist.h"
#include "containers/hashtable.h"

#include <vulkan/vulkan.h>

// checks the given expression's return value against VK_SUCCESS
#define VK_CHECK(expr)               \
    {                                \
        KASSERT(expr == VK_SUCCESS); \
    }

struct vulkan_context;

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
    b8 has_freelist;
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
    // @brief the chosed depth format's number of channels
    u8 depth_channel_count;
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

    f32 depth;    // store the info for the renderpass depth
    u32 stencil;  // need to look up, but has something to do with depth

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
    // @brief an array of pointers to render targets, which contain swapchain images
    texture** render_textures;

    // @brief the depth of the texture
    texture* depth_texture;

    // @brief render targets used for on screen rendering, one per frame.
    // the images contained in these are created and owned by the swapchain
    render_target render_targets[3];
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

// @brief max number of ui control instances
// @todo TODO: make configurable
#define VULKAN_MAX_UI_COUNT 1024

// @brief put some hard limits in place for the count of supported textures, attributes, uniforms, ect.
// this is to maintain memory locality and avoid dynamic allocations
// @brief the maximum number of stages (such as vert, frag, comp, ect.) allowed
#define VULKAN_SHADER_MAX_STAGES 8
// @brief the maximum number of textures allowed at the global level
#define VULKAN_SHADER_MAX_GLOBAL_TEXTURES 31
// @brief the maximum number of textures allowed at the instance level
#define VULKAN_SHADER_MAX_INSTANCE_TEXTURES 31
// @brief the maximum number of vertex input attributes allowed
#define VULKAN_SHADER_MAX_ATTRIBUTES 16

// @brief the maximum number of uniforms and samplers allowed at the
// global, instance and local levels combined. it's probably more than will ever be needed
#define VULKAN_SHADER_MAX_UNIFORMS 128

// @brief the maximum number of bindings per descriptor set
#define VULKAN_SHADER_MAX_BINDINGS 2
// @brief the maximum number of push constant ranges for a shader
#define VULKAN_SHADER_MAX_PUSH_CONST_RANGES 32

// @brief configuration for a shader stage, such as vertex or fragment
typedef struct vulkan_shader_stage_config {
    // @brief the shader stage bit flag
    VkShaderStageFlagBits stage;
    // @brief the shader file name
    char file_name[255];
} vulkan_shader_stage_config;

// @brief the configuration for a descriptor set
typedef struct vulkan_descriptor_set_config {
    // @brief the number of bindings in this set
    u8 binding_count;
    // @brief an array of binding layouts for this set.
    VkDescriptorSetLayoutBinding bindings[VULKAN_SHADER_MAX_BINDINGS];
} vulkan_descriptor_set_config;

// @brief internal shader configuration generated by vulkan_shader_create()
typedef struct vulkan_shader_config {
    // @brief the number of shader stages in this shader
    u8 stage_count;
    // @brief the configuration for every stage of this shader
    vulkan_shader_stage_config stages[VULKAN_SHADER_MAX_STAGES];
    // @brief an array of descriptor pool sizes
    VkDescriptorPoolSize pool_sizes[2];
    // @brief the max number fo descriptor sets that can be allocated from this shader.
    // should typically be a decently high number
    u16 max_descriptor_set_count;
    // @brief the total number of descriptor sets configured for this shader
    // is 1 if only using global uniforms/samplers, otherwise 2;
    u8 descriptor_set_count;
    // @brief descriptor sets, max of 2. index 0=global, 1=instance
    vulkan_descriptor_set_config descriptor_sets[2];
    // @brief an array of attribute descriptions for this shader
    VkVertexInputAttributeDescription attributes[VULKAN_SHADER_MAX_ATTRIBUTES];
} vulkan_shader_config;

// @brief represents a state for a given descriptor. this is used to determine when a descriptor
// needs updating. there is a state per frame (with a max of 3).
typedef struct vulkan_descriptor_state {
    // @brief the descriptor generation, per frame
    u8 generations[3];
    // @brief the identifier, per frame. typically used for texture ids
    u32 ids[3];
} vulkan_descriptor_state;

// @brief represents the state for a descriptor set. this is used to track generations and updates, potentially
// for optimization via skipping sets which do not need updating.
typedef struct vulkan_shader_descriptor_set_state {
    // @brief The descriptor sets for this instance, one per frame. */
    VkDescriptorSet descriptor_sets[3];
    // @brief a descriptor state per descriptor, which in turn handles frames. count is managed in shader config
    vulkan_descriptor_state descriptor_states[VULKAN_SHADER_MAX_BINDINGS];
} vulkan_shader_descriptor_set_state;

// @brief the instance level state for a shader
typedef struct vulkan_shader_instance_state {
    // @brief the instance id. invalid id if not used
    u32 id;
    // @brief the offset in bytes in the instance uniform buffer
    u64 offset;
    // @brief a state for the descriptor set
    vulkan_shader_descriptor_set_state descriptor_set_state;
    // @brief instance texture pointers, which are used during rendering. these are set by calls to set_sampler
    struct texture_map** instance_texture_maps;
} vulkan_shader_instance_state;

// @brief represents a generic vulkan shader. this uses a set of inputs and parameters, as well as the shader
// programs contained in spir-v files to construct a shader for use in rendering
typedef struct vulkan_shader {
    // @brief the block of memory mapped to the uniform buffer
    void* mapped_uniform_buffer_block;

    // @brief the shader identifier
    u32 id;

    // @brief the configuration of the shader generated by vulkan_create_shader()
    vulkan_shader_config config;

    // @brief a pointer to the renderpass to be used with this shader
    vulkan_renderpass* renderpass;

    // @brief an array of stages (such as vertex and fragment) for this shader. count is located in config
    vulkan_shader_stage stages[VULKAN_SHADER_MAX_STAGES];

    // @brief the descriptor pool used for this shader
    VkDescriptorPool descriptor_pool;

    // @brief descriptor set layouts, max of 2. index 0=global, 1=instance
    VkDescriptorSetLayout descriptor_set_layouts[2];
    // @brief global descriptor sets, one per frame
    VkDescriptorSet global_descriptor_sets[3];
    // @brief the uniform buffer used by this shader
    vulkan_buffer uniform_buffer;

    // @brief the pipeline associated with this shader
    vulkan_pipeline pipeline;

    // @brief the instance states for all instances. @todo TODO: make dynamic
    u32 instance_count;
    vulkan_shader_instance_state instance_states[VULKAN_MAX_MATERIAL_COUNT];

} vulkan_shader;

#define VULKAN_MAX_REGISTERED_RENDERPASSES 31

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

    vulkan_swapchain swapchain;  // vulkan swapchain - controls images to be rendered to and presented, hold the images

    void* renderpass_table_block;
    hashtable renderpass_table;

    // @brief registered renderpasses
    renderpass registered_passes[VULKAN_MAX_REGISTERED_RENDERPASSES];

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
    VkFence images_in_flight[3];  // possibly they are in in flight fences at the moment, keeps track of them

    u32 image_index;    // to keep track of images in the swap chain - index of the image that we are currently using
    u32 current_frame;  // keep track of the frames

    b8 recreating_swapchain;  // a state that needs to be tracked in the render loop

    // TODO: make dynamic
    vulkan_geometry_data geometries[VULKAN_MAX_GEOMETRY_COUNT];

    // @brief render targets used for world rendering. @note one per frame
    render_target world_render_targets[3];

    i32 (*find_memory_index)(u32 type_filter, u32 property_flags);  // a fuction pointer that takes in a type filter and the property flags - returns a 32 bit int

    // @brief a pointer to a function to be called when the backend requires rendertargets to be refreshed/regenerated
    void (*on_rendertarget_refresh_required)();

} vulkan_context;