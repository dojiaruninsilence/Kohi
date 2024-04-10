#include "vulkan_backend.h"

#include "vulkan_types.inl"
#include "vulkan_platform.h"
#include "vulkan_device.h"
#include "vulkan_swapchain.h"
#include "vulkan_renderpass.h"
#include "vulkan_command_buffer.h"
#include "vulkan_utils.h"
#include "vulkan_buffer.h"
#include "vulkan_image.h"

#include "core/logger.h"
#include "core/kstring.h"
#include "core/kmemory.h"
#include "core/application.h"

#include "containers/darray.h"

#include "math/math_types.h"

#include "platform/platform.h"

// shaders
#include "shaders/vulkan_material_shader.h"
#include "shaders/vulkan_ui_shader.h"

#include "systems/material_system.h"

// create vulkan context - there will only be one
static vulkan_context context;

// static variables -- uses until we get a replacement value
static u32 cached_framebuffer_width = 0;
static u32 cached_framebuffer_height = 0;

// foreward declaration
VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,    // takes in the severity of the message
    VkDebugUtilsMessageTypeFlagsEXT message_types,              // the type of message
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,  // the message
    void* user_data);                                           // could contain the name of the application or wherever it is running from

// foreward declarations
i32 find_memory_index(u32 type_filter, u32 property_flags);
b8 create_buffers(vulkan_context* context);  // private function declaration to create a data buffer

void create_command_buffers(renderer_backend* backend);  // private function to create command buffers, just takes in the renderer backend
void regenerate_framebuffers();                          // private function to create/regenerate framebuffers, pass in pointers to the backend, the swapchain, and the renderpass - going to hook all these together
b8 recreate_swapchain(renderer_backend* backend);        // private function to recreate the swapchain, just takes in a pointer to the backend

// temporary funtion to test if everything is working correctly
void upload_data_range(vulkan_context* context, VkCommandPool pool, VkFence fence, VkQueue queue, vulkan_buffer* buffer, u64 offset, u64 size, const void* data) {
    // Create a host-visible staging buffer to upload to. Mark it as the source of the transfer.
    VkBufferUsageFlags flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    vulkan_buffer staging;
    vulkan_buffer_create(context, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, flags, true, &staging);

    // Load the data into the staging buffer.
    vulkan_buffer_load_data(context, &staging, 0, size, 0, data);

    // Perform the copy from staging to the device local buffer.
    vulkan_buffer_copy_to(context, pool, fence, queue, staging.handle, 0, buffer->handle, offset, size);

    // Clean up the staging buffer.
    vulkan_buffer_destroy(context, &staging);
}

void free_data_range(vulkan_buffer* buffer, u64 offset, u64 size) {
    // TODO: free this in the buffer
    // TODO: update the free list with this range being freed
}

b8 vulkan_renderer_backend_initialize(renderer_backend* backend, const char* application_name) {
    // function pointers
    context.find_memory_index = find_memory_index;

    // TODO: custom allocator
    context.allocator = 0;  // zero out the custom allocator

    // get the frame buffer size from the size stored in the app state - store the values in cached frame buffer width and height
    application_get_framebuffer_size(&cached_framebuffer_width, &cached_framebuffer_height);
    // check to make sure that the cached frambuffer size has actually been fillew out, other wise use default values
    context.framebuffer_width = (cached_framebuffer_width != 0) ? cached_framebuffer_width : 800;     // if the cached frame buffer width is not zero then use it for the framebuffer width, otherwise use 800
    context.framebuffer_height = (cached_framebuffer_height != 0) ? cached_framebuffer_height : 600;  // if the cached frame buffer height is not zero then use it for the framebuffer height, otherwise use 600
    // reset the cached framebuffer size
    cached_framebuffer_width = 0;
    cached_framebuffer_height = 0;

    // setup the vulkan instance -- wnen ever you create an instance handle in vulkan there is creation info that has to go with it
    VkApplicationInfo app_info = {VK_STRUCTURE_TYPE_APPLICATION_INFO};  // gives vulkan some info about the app itself
    app_info.apiVersion = VK_API_VERSION_1_2;                           // which version of the api that we are targeting
    app_info.pApplicationName = application_name;                       // tell vulkan what the name of the application
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);             // tell vulkan the version of our app
    app_info.pEngineName = "Kohi Engine";                               // tell vulkan the name of our engine
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);                  // tell vulkan the version of out engine

    // create our vk instance
    // first have to create the information for the instance creation
    VkInstanceCreateInfo create_info = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};  // first have to create the info it is going to need -- will see this pattern alot in vulkan, always pass in the type first, which will match the function name
    create_info.pApplicationInfo = &app_info;                                     // pass the address to the info that was created above

    // obtain a list of all required extensions
    const char** required_extensions = darray_create(const char*);         // create an array for the required extensions and allocate the memory for it -- why 2 pointer symbols
    darray_push(required_extensions, &VK_KHR_SURFACE_EXTENSION_NAME);      // generic surface extension vulkan uses to render surfaces to - push the required extensions into the array
    platform_get_required_extension_names(&required_extensions);           // get the platform specific extensions -- pass the address of the required extensions array
#if defined(_DEBUG)                                                        // if its in debug mode
    darray_push(required_extensions, &VK_EXT_DEBUG_UTILS_EXTENSION_NAME);  // push the debug extention into the required extensions array

    KDEBUG("Required extensions:");
    u32 length = darray_length(required_extensions);  // set length to the number of elements in the array
    for (u32 i = 0; i < length; ++i) {                // iterate through the array
        KDEBUG(required_extensions[i]);               // log each extension in the array
    }
#endif

    // will be going over these in the next video, but they do need to be zeroed out for now
    create_info.enabledExtensionCount = darray_length(required_extensions);  // set the extension count to the number of elements in the array
    create_info.ppEnabledExtensionNames = required_extensions;               // set the enabled extentions names to the array of extention names - tells vulkan to load those extensions

    // validation layers --  used for debug mode
    const char** required_validation_layer_names = 0;  // set to zero because they may not be used
    u32 required_validation_layer_count = 0;           // set to zero because they may not be used

// if validation should be done, get a list of the required validation layer names and make sure that they exsist. validation layers should only be enabled on non release builds
#if defined(_DEBUG)
    KINFO("Validation layers enabled. Enumerating...");

    // the list of validation layers required
    required_validation_layer_names = darray_create(const char*);                      // create the array and allocate memory for it
    darray_push(required_validation_layer_names, &"VK_LAYER_KHRONOS_validation");      // push in the khronos validation layer into the array
    required_validation_layer_count = darray_length(required_validation_layer_names);  // layer count set to the number of elements in the array

    // obtain a list of available validation layers
    u32 available_layer_count = 0;
    VK_CHECK(vkEnumerateInstanceLayerProperties(&available_layer_count, 0));                         // query the available layers
    VkLayerProperties* available_layers = darray_reserve(VkLayerProperties, available_layer_count);  // allocate memory for the available layers array
    VK_CHECK(vkEnumerateInstanceLayerProperties(&available_layer_count, available_layers));          // pass available layers into the available layers count -- gives us the total list of available layers

    // verify that all required layers are available
    for (u32 i = 0; i < required_validation_layer_count; ++i) {  // iterate through the required layers
        KINFO("Searching for layer: %s...", required_validation_layer_names[i]);
        b8 found = false;
        for (u32 j = 0; j < available_layer_count; ++j) {                                            // iterate through the available layers
            if (strings_equal(required_validation_layer_names[i], available_layers[j].layerName)) {  // if both are the same
                found = true;
                KINFO("Found. ");
                break;
            }
        }

        if (!found) {  // if a required layer isnt in the list of available
            KFATAL("Required validation layer is missing: %s", required_validation_layer_names[i]);
            return false;
        }
    }
    KINFO("All validation layers are present.");
#endif

    create_info.enabledLayerCount = required_validation_layer_count;    // set the layer count to the required layer count
    create_info.ppEnabledLayerNames = required_validation_layer_names;  // set the layer names to the required layer names array

    // call the actual creation method
    VK_CHECK(vkCreateInstance(&create_info, context.allocator, &context.instance));  // pass in the info created above, the custom allocator(default for now), and the address to where the context handle is being stored. VKCHECK  checks success and throws assert on fail
    KINFO("Vulkan instance created.");

// vulkan debugger
#if defined(_DEBUG)  // obviously it only works in debug mode
    KDEBUG("Creating Vulkan debuger...");
    u32 log_severity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;  // the sevarity levels that we are going to use from those used by vulkan
    // | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT; // other severities that vulkan uses, may use later, may not

    VkDebugUtilsMessengerCreateInfoEXT debug_create_info = {VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};                                                                // create the info for the extension we are about to create
    debug_create_info.messageSeverity = log_severity;                                                                                                                                // pass in the severity
    debug_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;  // filters for what message types we actually want
    debug_create_info.pfnUserCallback = vk_debug_callback;                                                                                                                           // what it does with the logging request it recieves -- handler for any messages and such

    PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(context.instance, "vkCreateDebugUtilsMessengerEXT");  // call the debug messanger, but is not reularly loaded with vulkan so have to load a function pointer to it
    KASSERT_MSG(func, "Failed to create debug messanger!");                                                                                                   // shut down with a message
    VK_CHECK(func(context.instance, &debug_create_info, context.allocator, &context.debug_messenger));                                                        // also run the function against the vk_ssuccess
    KDEBUG("Vulkan debugger created.");
#endif

    // vulkan surface creation
    KDEBUG("Creating Vulkan surface...");
    if (!platform_create_vulkan_surface(&context)) {   // create surface, and check to see if it worked
        KERROR("Failed to create platform surface!");  // let us know if it failed
        return false;
    }
    KDEBUG("Vulkan surface created.");

    // vulkan device creation
    if (!vulkan_device_create(&context)) {   // if it fails to create
        KERROR("Failed to create device!");  // trhow error
        return false;                        // boot out
    }

    // vulkan swapchain creation
    vulkan_swapchain_create(
        &context,                    // pass in address to the context
        context.framebuffer_width,   // pass in a width
        context.framebuffer_height,  // pass in a height
        &context.swapchain);         // address to the swapchain being created

    // vulkan renderpass creation - world renderpass
    vulkan_renderpass_create(
        &context,                                                                                                         // pass in address to context
        &context.main_renderpass,                                                                                         // pass in the main renderpass
        (vec4){0, 0, context.framebuffer_width, context.framebuffer_height},                                              // pass in an x, y, width and height - render area
        (vec4){0.0f, 0.0f, 0.2f, 1.0f},                                                                                   // pass in rgba values - clear color(dark blue)
        1.0f,                                                                                                             // pass in a depth
        0,                                                                                                                // pass in a stencil
        RENDERPASS_CLEAR_COLOUR_BUFFER_FLAG | RENDERPASS_CLEAR_DEPTH_BUFFER_FLAG | RENDERPASS_CLEAR_STENCIL_BUFFER_FLAG,  // flags for how clearing is handled
        false, true);                                                                                                     // is the first renderpass, so none before it, and there are going to be more after it

    // ui renderpass
    vulkan_renderpass_create(
        &context,
        &context.ui_renderpass,                                               // ui renderpass
        (vec4){0, 0, context.framebuffer_width, context.framebuffer_height},  // render area
        (vec4){0.0f, 0.0f, 0.0f, 0.0f},                                       // clear color black and fully transparent, not important
        1.0f,                                                                 // depth
        0,                                                                    // no stencil for now
        RENDERPASS_CLEAR_NONE_FLAG,                                           // isnt a fist pass so dont clear anything
        true, false);                                                         // is not the first pass, and there are no passes after this one yet

    // create swapchain frame buffers
    regenerate_framebuffers();  // call our private function regenerate framebuffers, pass in the backend, and addresses to the swapchain, and the main renderpass

    // create command buffers
    create_command_buffers(backend);

    // create syncronization objects
    context.image_available_semaphores = darray_reserve(VkSemaphore, context.swapchain.max_frames_in_flight);  // create a darray and allocated memory for image available semaphores. use the size of a vulkan semaphore times the max frames in flight in the swapchain
    context.queue_complete_semaphores = darray_reserve(VkSemaphore, context.swapchain.max_frames_in_flight);   // create a darray and allocated memory for queue complete semaphores. use the size of a vulkan semaphore times the max frames in flight in the swapchain

    for (u8 i = 0; i < context.swapchain.max_frames_in_flight; ++i) {  // iterate through the max frames in flight
        // create a vulkan semaphore create info struct, and use the provided macro to input default values into the fields
        VkSemaphoreCreateInfo semaphore_create_info = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        // use the vulkan to create both image availabel semaphores and queue complete semaphores for image in flight at index i, pass in the logical device and the info created above, the memory allocation stuffs, and the address to the semaphore being created
        vkCreateSemaphore(context.device.logical_device, &semaphore_create_info, context.allocator, &context.image_available_semaphores[i]);
        vkCreateSemaphore(context.device.logical_device, &semaphore_create_info, context.allocator, &context.queue_complete_semaphores[i]);

        // create the fence in  a signaled state, indicating that the first frame has already been "rendered". this will prevent the application from waiting indefinately for the first frame to render
        // since it cannot be rendered until a frame is "rendered" before it
        VkFenceCreateInfo fence_create_info = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        VK_CHECK(vkCreateFence(context.device.logical_device, &fence_create_info, context.allocator, &context.in_flight_fences[i]));
    }

    // create the in flight images
    // in flight fences should not yet exist at this point, so clear the list. these are stored in pointers, because the initial state should be 0, and will be 0 when not in use
    // Actual fences are not owned by this list
    for (u32 i = 0; i < context.swapchain.image_count; ++i) {  // iterate through the swapchain images
        context.images_in_flight[i] = 0;                       // and make sure that all of the images in flight are zeroed out - nothing should be in flight jaust yet
    }

    // create built in shaders
    // create an object shader, pass in addresses to the context and the object shader
    if (!vulkan_material_shader_create(&context, &context.material_shader)) {  // if it fails
        KERROR("Error loading built-in basic_lighting shader.");               // throw an error
        return false;                                                          // and boot out
    }
    // create the ui shader
    if (!vulkan_ui_shader_create(&context, &context.ui_shader)) {
        KERROR("Error loading built-in ui shader.");
        return false;
    }

    // create buffers
    create_buffers(&context);

    // mark all geometries as invalid
    for (u32 i = 0; i < VULKAN_MAX_GEOMETRY_COUNT; ++i) {
        context.geometries[i].id = INVALID_ID;
    }

    // everything passed
    KINFO("Vulkan renderer initialized successfully.");
    return true;
}

void vulkan_renderer_backend_shutdown(renderer_backend* backend) {
    // wait until the device is doing nothing at all
    vkDeviceWaitIdle(context.device.logical_device);

    // destroy in the opposite order of creation

    // destroy the vulkan buffers
    vulkan_buffer_destroy(&context, &context.object_vertex_buffer);
    vulkan_buffer_destroy(&context, &context.object_index_buffer);

    // destroy the ui shader
    vulkan_ui_shader_destroy(&context, &context.ui_shader);

    // destroy the vulkan object shader
    vulkan_material_shader_destroy(&context, &context.material_shader);  // pass in addresses to the context, and the object shader to be destoyed

    // destroy syncronization objects
    for (u8 i = 0; i < context.swapchain.max_frames_in_flight; ++i) {  // iterate through all the max frames in flight
        if (context.image_available_semaphores[i]) {                   // if there is a semaphore at index i
            vkDestroySemaphore(                                        // call the vulkan function to destroy a semaphore
                context.device.logical_device,                         // pass it the logical device
                context.image_available_semaphores[i],                 // the semaphore to be destroyed
                context.allocator);                                    // and the memory allocation stuffs
            context.image_available_semaphores[i] = 0;                 // set index i to 0
        }
        if (context.queue_complete_semaphores[i]) {    // if there is a semaphore at index i
            vkDestroySemaphore(                        // call the vulkan function to destroy a semaphore
                context.device.logical_device,         // pass it the logical device
                context.queue_complete_semaphores[i],  // the semaphore to be destroyed
                context.allocator);                    // and the memory allocation stuffs
            context.queue_complete_semaphores[i] = 0;  // set index i to 0
        }
        vkDestroyFence(context.device.logical_device, context.in_flight_fences[i], context.allocator);  // use vulkan function to destroy the fence at inde i, needs an address to the context, and an address to the fence being destroyed
    }
    darray_destroy(context.image_available_semaphores);  // destroy the image available semaphores darray and free the memory
    context.image_available_semaphores = 0;              // set the value of the destroyed darray to 0

    darray_destroy(context.queue_complete_semaphores);  // destroy the image available semaphores darray and free the memory
    context.queue_complete_semaphores = 0;              // set the value of the destroyed darray to 0

    // destroy the command buffers
    for (u32 i = 0; i < context.swapchain.image_count; ++i) {  // iterate through all of the swapchain images
        if (context.graphics_command_buffers[i].handle) {      // if a handle exists at index i
            vulkan_command_buffer_free(                        // call our function to return the command buffer to the command pool
                &context,                                      // takes in an address to the context
                context.device.graphics_command_pool,          // takes in the pool the command buffer belongs to
                &context.graphics_command_buffers[i]);         // and the command buffer at index i
            context.graphics_command_buffers[i].handle = 0;    // set the hendle at index i to zero
        }
    }
    darray_destroy(context.graphics_command_buffers);  // destroy the darray
    context.graphics_command_buffers = 0;              // set the command buffers to zero

    // destroy the framebuffers
    for (u32 i = 0; i < context.swapchain.image_count; ++i) {  // iterate through the swapchain images
        vkDestroyFramebuffer(context.device.logical_device, context.world_framebuffers[i], context.allocator);
        vkDestroyFramebuffer(context.device.logical_device, context.swapchain.framebuffers[i], context.allocator);  // use vk function to destroy the framebuffer, pass in the address to the context, and the address to the frambuffer at index i
    }

    // destroy renderpasses
    // destroy the ui renderpass
    vulkan_renderpass_destroy(&context, &context.ui_renderpass);
    // destroy the world render pass
    vulkan_renderpass_destroy(&context, &context.main_renderpass);

    // destroy the swapchain
    vulkan_swapchain_destroy(&context, &context.swapchain);

    // destroy the vulkan device
    KDEBUG("Destroying Vulkan Device...");
    vulkan_device_destroy(&context);

    // destroy the vulkan surface
    KDEBUG("Destroying Vulkan surface...");
    if (context.surface) {                                                          // if there is a surface
        vkDestroySurfaceKHR(context.instance, context.surface, context.allocator);  // destroy the vulkan surface utilizing a vulkan function
        context.surface = 0;                                                        // set the surface to zero
    }

#if defined(_DEBUG)  // if in debug mode
    // destroy the vulkan debugger
    KDEBUG("Destroying Vulkan debugger...");
    if (context.debug_messenger) {                                                                                                                                   // check that it exists
        PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(context.instance, "vkDestroyDebugUtilsMessengerEXT");  // get a function pointer to destroy debug messenger ext func
        func(context.instance, context.debug_messenger, context.allocator);                                                                                          // then call the function
    }
#endif

    // destroy the vulkan instance
    KDEBUG("Destroying Vulkan instance...");
    vkDestroyInstance(context.instance, context.allocator);  // use vulkan function to destroy the instance
}

void vulkan_renderer_backend_on_resized(renderer_backend* backend, u16 width, u16 height) {
    // update the "framebuffer size generation", a counter which indicates when the framebuffer size has been updated
    cached_framebuffer_width = width;       // pass in the width
    cached_framebuffer_height = height;     // pass in the height
    context.framebuffer_size_generation++;  // increment the frambuffer size generation - whenever this changes we know that we have recieved a resize event

    KINFO("Vulkan renderer backend->resized: w/h/gen: %i/%i/%llu", width, height, context.framebuffer_size_generation);
}

b8 vulkan_renderer_backend_begin_frame(renderer_backend* backend, f32 delta_time) {
    // store the delta time for later processes
    context.frame_delta_time = delta_time;
    vulkan_device* device = &context.device;  // just to make writting the code a little bit easier and cleaner - conveneience pointer

    // check if recreating swap chain and boot out
    if (context.recreating_swapchain) {                                                                                           // is context in a recreating swapchain state
        VkResult result = vkDeviceWaitIdle(device->logical_device);                                                               // run the vulkan function to wait until the device is idle and store results in result
        if (!vulkan_result_is_success(result)) {                                                                                  // use function to determine the success of result, if failed
            KERROR("vulkan_renderer_backend_begin_frame vkDeviceWaitIdle (1) failed: '%s'", vulkan_result_string(result, true));  // throw error using a string from result to fill out message
            return false;                                                                                                         // boot out
        }
        // if waited successfully
        KINFO("Recreating swapchain, booting.");
        return false;
    }

    // check if the framebuffer has been resized. if so, a new swapchain must be created
    if (context.framebuffer_size_generation != context.framebuffer_size_last_generation) {                                        // if the frambuffer size generation is not the same as last generation, the window has been resized
        VkResult result = vkDeviceWaitIdle(device->logical_device);                                                               // run the vulkan function to wait until the device is idle and store results in result
        if (!vulkan_result_is_success(result)) {                                                                                  // use function to determine the success of result, if failed
            KERROR("vulkan_renderer_backend_begin_frame vkDeviceWaitIdle (2) failed: '%s'", vulkan_result_string(result, true));  // throw error using a string from result to fill out message
            return false;                                                                                                         // boot out
        }

        // if the swapchain recreation failec(because, for example, the window was minimized), boot out before unsetting the flag
        if (!recreate_swapchain(backend)) {  // run the function to recreate the swapchain, and if it fails
            return false;                    // boot out
        }

        KINFO("Resized, booting.");
        return false;
    }

    // if no resizing and no swapchain recreation then
    // wait for the execution of the current frame to complete. the fence being free will allow this one to move on
    VkResult result = vkWaitForFences(context.device.logical_device, 1, &context.in_flight_fences[context.current_frame], true, UINT64_MAX);
    if (!vulkan_result_is_success(result)) {
        KERROR("In-flight fence wait failure! error: %s", vulkan_result_string(result, true));
        return false;
    }

    // aquire the next image from the swap chain. pass along the semaphore that should be signaled when this completes
    // this same semaphore will later be waited on by the queue submission to ensure this image is available
    if (!vulkan_swapchain_acquire_next_image_index(                     // call the function to aquire the next image index
            &context,                                                   // pass an address to the context
            &context.swapchain,                                         // and an address to the swapchain
            UINT64_MAX,                                                 // a bogus high value
            context.image_available_semaphores[context.current_frame],  // the image available semaphore that is attached to the current frame - and should be signaled when this completes use current frame to sync them up
            0,                                                          // no time out
            &context.image_index)) {                                    // and the index of the image being aquired
        return false;
    }

    // begin recording commands
    vulkan_command_buffer* command_buffer = &context.graphics_command_buffers[context.image_index];  // convenience pointer, to svae time and cleaner code
    vulkan_command_buffer_reset(command_buffer);                                                     // reset the command buffer at image index to the ready state
    vulkan_command_buffer_begin(command_buffer, false, false, false);                                // begin the command buffer with single use, renderpass continuos, and simultaneous use to false

    // set the viewport
    // dynamic state - must be set every frame
    VkViewport viewport;                                 // define a vulkan viewport
    viewport.x = 0.0f;                                   // starts at 0.0 on the x axis  -- vulkan by default is 0, 0 at the top left corner
    viewport.y = (f32)context.framebuffer_height;        // starts at the height of the framebuffer for the y, which flips the viewport, so that it is consistant with opengl
    viewport.width = (f32)context.framebuffer_width;     // pass in the width from the framebuffer
    viewport.height = -(f32)context.framebuffer_height;  // pass in the height from the framebuffer and make negative, again to flip the y axis to match opengl
    viewport.minDepth = 0.0f;                            // z values
    viewport.maxDepth = 1.0f;                            // z values

    // scissor, this determines what parts get rendered and what gets clipped
    VkRect2D scissor;                                    // define a vulkan rectangle and call scissor
    scissor.offset.x = scissor.offset.y = 0;             // set the scissor offset for the x and they y to 0
    scissor.extent.width = context.framebuffer_width;    // pass in width from the frame buffer
    scissor.extent.height = context.framebuffer_height;  // pass in the height from the frame buffer

    // issue commands
    vkCmdSetViewport(command_buffer->handle, 0, 1, &viewport);  // call a vulkan function to set the viewport, pass it a handle to the command buffer for the current frame, vieport index is zero, one viewport, and address to the viewport we setup
    vkCmdSetScissor(command_buffer->handle, 0, 1, &scissor);    // call a vulkan function to clip everything that is outside of the viewport, pass it the handle to the current framebuffer, viewport index is zero, one viewport, and the scissor rect created above

    // set the width and height of the main renderpass by passing in values from the framebuffer -- may be uneccassary to do every frame
    context.main_renderpass.render_area.z = context.framebuffer_width;
    context.main_renderpass.render_area.w = context.framebuffer_height;

    return true;
}

// update the global state, this will probably control camera movements and such, pass in a view, and projection matrices, the view position, ambient color and the mode
// none of these are pointers, here we are making copies, so the rest of the engine can keep doing its thing, and mot waiting for this to finish
void vulkan_renderer_update_global_world_state(mat4 projection, mat4 view, vec3 view_position, vec4 ambient_colour, i32 mode) {
    vulkan_command_buffer* command_buffer = &context.graphics_command_buffers[context.image_index];  // store a pointer to the current graphics command buffer

    // use the shader
    vulkan_material_shader_use(&context, &context.material_shader);

    // pass through the view, and projection
    context.material_shader.global_ubo.projection = projection;
    context.material_shader.global_ubo.view = view;

    // TODO: other ubo properties

    // update the global state
    vulkan_material_shader_update_global_world_state(&context, &context.material_shader, context.frame_delta_time);
}

// update the global ui state
void vulkan_renderer_update_global_ui_state(mat4 projection, mat4 view, i32 mode) {
    vulkan_command_buffer* command_buffer = &context.graphics_command_buffers[context.image_index];  // store a pointer to the current graphics command buffer

    // use the shader
    vulkan_ui_shader_use(&context, &context.ui_shader);

    // pass through the view, and projection
    context.ui_shader.global_ubo.projection = projection;
    context.ui_shader.global_ubo.view = view;

    // TODO: other ubo properties

    // update the global state
    vulkan_ui_shader_update_global_state(&context, &context.ui_shader, context.frame_delta_time);
}

b8 vulkan_renderer_backend_end_frame(renderer_backend* backend, f32 delta_time) {
    vulkan_command_buffer* command_buffer = &context.graphics_command_buffers[context.image_index];  // conveinience pointer

    // end the command buffer- calling our function. just pass in the current graphics command buffer
    vulkan_command_buffer_end(command_buffer);

    // make sure the previous frame is not using this image (i.e. its fence is being waited on)
    if (context.images_in_flight[context.image_index] != VK_NULL_HANDLE) {  // if there is acctually in images in flight at image index
        VkResult result = vkWaitForFences(context.device.logical_device, 1, context.images_in_flight[context.image_index], true, UINT64_MAX);
        if (!vulkan_result_is_success(result)) {
            KERROR("vk_fence_wait error: %s", vulkan_result_string(result, true));
            return false;
        }
    }

    // mark the image fence as in use by this frame
    context.images_in_flight[context.image_index] = &context.in_flight_fences[context.current_frame];  // assign the current inflight fence to the image in flight at image index

    // reset the fence for use on the next frame
    VK_CHECK(vkResetFences(context.device.logical_device, 1, &context.in_flight_fences[context.current_frame]));  // use vk function to reset a fence, pass in the address to the context, the address to the in flight fence of the current frame

    // call to submit the work
    // submit the queue and wait for the operation to complete
    // begin queue submission
    VkSubmitInfo submit_info = {VK_STRUCTURE_TYPE_SUBMIT_INFO};  // create a submit info struct and use the provided macro to fill it with default values

    // command buffer(s) to be executed
    submit_info.commandBufferCount = 1;                     // only one command buffer for now
    submit_info.pCommandBuffers = &command_buffer->handle;  // pass in the address to the command buffer handle

    // the semaphore(s) to be signaled when the queue is complete
    submit_info.signalSemaphoreCount = 1;                                                       // only using one semaphore for now
    submit_info.pSignalSemaphores = &context.queue_complete_semaphores[context.current_frame];  // pass in the address of the queue complete semaphore at the index of the current frame

    // wait semaphore ensures that the operation cannot begin until the image is available.
    submit_info.waitSemaphoreCount = 1;                                                        // only using one wait semaphore for now
    submit_info.pWaitSemaphores = &context.image_available_semaphores[context.current_frame];  // pass in the address of the image available semaphore at the index of the current frame

    // this is how we ensure that only one of the triple buffers gets displayed
    // each semaphore waits on the corresponding pipeline stage to complete. 1:1 ratio.
    // VK_PIPLINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT prvents subsequent colour attachment writes from executing until the semaphore signals (i.e. one frame is presented at a time)
    VkPipelineStageFlags flags[1] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};  // eventually there will be more here, this accepts an array
    submit_info.pWaitDstStageMask = flags;                                            // pass all the flags in

    VkResult result = vkQueueSubmit(                       // run the vulkan function to submit a queue and store the results in result
        context.device.graphics_queue,                     // pass in the graphics queue
        1,                                                 // only submitting one queue for now
        &submit_info,                                      // pass in the info struct we just created
        context.in_flight_fences[context.current_frame]);  // and the handle to the in flight fence at the index of the current frame

    // check the results of submit
    if (result != VK_SUCCESS) {                                                              // if the submit was unsuccessful
        KERROR("vkQueueSubmit failed with result: %s", vulkan_result_string(result, true));  // throw out an error message, with the info of failure
        return false;
    }

    // update the command buffer
    vulkan_command_buffer_update_submitted(command_buffer);  // change the current command buffer's state to submitted

    // end the queue submission

    // here is where it is drawn to the screen
    // give the image back to the swapchain
    vulkan_swapchain_present(                                      // call our function to present the swapchain
        &context,                                                  // pass in an address to the context
        &context.swapchain,                                        // an address to the swapchain
        context.device.graphics_queue,                             // the graphics queue
        context.device.present_queue,                              // the present queue
        context.queue_complete_semaphores[context.current_frame],  // the queue complete semaphore of the index of the current frame
        context.image_index);                                      // and the image index

    // if all of this has passed, we have rendered to the screen
    return true;
}

// begin a render pass
b8 vulkan_renderer_begin_renderpass(struct renderer_backend* backend, u8 renderpass_id) {
    vulkan_renderpass* renderpass = 0;
    VkFramebuffer framebuffer = 0;
    vulkan_command_buffer* command_buffer = &context.graphics_command_buffers[context.image_index];

    // choose a renderpass based on id
    switch (renderpass_id) {
        case BUILTIN_RENDERPASS_WORLD:
            renderpass = &context.main_renderpass;
            framebuffer = context.world_framebuffers[context.image_index];
            break;
        case BUILTIN_RENDERPASS_UI:
            renderpass = &context.ui_renderpass;
            framebuffer = context.swapchain.framebuffers[context.image_index];
            break;
        default:
            KERROR("vulkan_renderer_begin_renderpass called on unrecognized id: %#02x", renderpass_id);
            return false;
    }

    // begin the render pass
    vulkan_renderpass_begin(command_buffer, renderpass, framebuffer);

    // use the appropriate shader
    switch (renderpass_id) {
        case BUILTIN_RENDERPASS_WORLD:
            vulkan_material_shader_use(&context, &context.material_shader);
            break;
        case BUILTIN_RENDERPASS_UI:
            vulkan_ui_shader_use(&context, &context.ui_shader);
            break;
    }

    return true;
}

// end a render pass
b8 vulkan_renderer_end_renderpass(struct renderer_backend* backend, u8 renderpass_id) {
    vulkan_renderpass* renderpass = 0;
    vulkan_command_buffer* command_buffer = &context.graphics_command_buffers[context.image_index];

    // choose a renderpass based on id
    switch (renderpass_id) {
        case BUILTIN_RENDERPASS_WORLD:
            renderpass = &context.main_renderpass;
            break;
        case BUILTIN_RENDERPASS_UI:
            renderpass = &context.ui_renderpass;
            break;
        default:
            KERROR("vulkan_renderer_end_renderpass called on unrecognised renderpass id:  %#02x", renderpass_id);
            return false;
    }

    vulkan_renderpass_end(command_buffer, renderpass);
    return true;
}

VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,    // takes in the severity of the message
    VkDebugUtilsMessageTypeFlagsEXT message_types,              // the type of message
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,  // the message
    void* user_data) {                                          // could contain the name of the application or wherever it is running from
    switch (message_severity) {                                 // takes the message sevarity and logs it at an appropriate level
        default:
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            KERROR(callback_data->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            KWARN(callback_data->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            KINFO(callback_data->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            KTRACE(callback_data->pMessage);
            break;
    }
    return VK_FALSE;  // this function is always supposed to return false
}

i32 find_memory_index(u32 type_filter, u32 property_flags) {
    VkPhysicalDeviceMemoryProperties memory_properties;                                       // create a memory properties struct
    vkGetPhysicalDeviceMemoryProperties(context.device.physical_device, &memory_properties);  // get the memory properties from the devie and fill into the struct

    for (u32 i = 0; i < memory_properties.memoryTypeCount; ++i) {  // iterate through the memory types
        // check each memory type to see if its bit is set to 1
        if (type_filter & (1 << i) && (memory_properties.memoryTypes[i].propertyFlags & property_flags) == property_flags) {  // checks to see if they match the flags that we passed in
            return i;
        }
    }

    KWARN("Unable to find suitable memory type!");
    return -1;
}

void create_command_buffers(renderer_backend* backend) {  // private function to create command buffers, just takes in the renderer backend
    // need to create a command buffer for each of our swapchain images - because we do things in the swapchain asyncronously
    if (!context.graphics_command_buffers) {                                                                      // if there is nothing stored in graphics command buffers
        context.graphics_command_buffers = darray_reserve(vulkan_command_buffer, context.swapchain.image_count);  // create a darray and reserve a block of memory for and array of vulkan command buffers numbering the amount of images in the swapchain
        for (u32 i = 0; i < context.swapchain.image_count; ++i) {                                                 // iterate through all of the swapchain images
            kzero_memory(&context.graphics_command_buffers[i], sizeof(vulkan_command_buffer));                    // zero out the memory in the alloted block at each of indices for the command buffers
        }
    }

    for (u32 i = 0; i < context.swapchain.image_count; ++i) {  // iterate through all of the swapchain images
        if (context.graphics_command_buffers[i].handle) {      // if there is a command buffer at the index i
            vulkan_command_buffer_free(                        // call our vulkan command buffer free function
                &context,                                      // pass in an address to the context
                context.device.graphics_command_pool,          // pass in the graphics command pool for th epool
                &context.graphics_command_buffers[i]);         // and freeing command buffer at index i
        }
        kzero_memory(&context.graphics_command_buffers[i], sizeof(vulkan_command_buffer));  // zero out the memory for command buffer i
        vulkan_command_buffer_allocate(                                                     // call our vulkan command buffer allocat function
            &context,                                                                       // passs in an adress to the context
            context.device.graphics_command_pool,                                           // pass in the graphics pool for the pool
            true,                                                                           // set primary to true
            &context.graphics_command_buffers[i]);                                          // and and address to the command buffer being allocated
    }

    KINFO("Vulkan surface created.");
}

// private function to create/regenerate framebuffers, pass in pointers to the backend, the swapchain, and the renderpass - going to hook all these together
void regenerate_framebuffers() {
    u32 image_count = context.swapchain.image_count;
    for (u32 i = 0; i < image_count; ++i) {  // iterate through all the swap chain image count. - we need a framebuffer for each swapchain image
        // world framebuffers
        // create and fill out the struct for creating framebuffers, start with default values using the vulkan macro
        VkImageView world_attachments[2] = {context.swapchain.views[i], context.swapchain.depth_attachment.view};
        VkFramebufferCreateInfo framebuffer_create_info = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        framebuffer_create_info.renderPass = context.main_renderpass.handle;
        framebuffer_create_info.attachmentCount = 2;
        framebuffer_create_info.pAttachments = world_attachments;
        framebuffer_create_info.width = context.framebuffer_width;
        framebuffer_create_info.height = context.framebuffer_height;
        framebuffer_create_info.layers = 1;

        VK_CHECK(vkCreateFramebuffer(context.device.logical_device, &framebuffer_create_info, context.allocator, &context.world_framebuffers[i]));

        // swapchain framebuffers (ui pass). outputs to swapchain images
        // create and fill out the struct for creating framebuffers, start with default values using the vulkan macro
        VkImageView ui_attachments[1] = {context.swapchain.views[i]};
        VkFramebufferCreateInfo sc_framebuffer_create_info = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        sc_framebuffer_create_info.renderPass = context.ui_renderpass.handle;
        sc_framebuffer_create_info.attachmentCount = 1;
        sc_framebuffer_create_info.pAttachments = ui_attachments;
        sc_framebuffer_create_info.width = context.framebuffer_width;
        sc_framebuffer_create_info.height = context.framebuffer_height;
        sc_framebuffer_create_info.layers = 1;

        VK_CHECK(vkCreateFramebuffer(context.device.logical_device, &sc_framebuffer_create_info, context.allocator, &context.swapchain.framebuffers[i]));
    }
}

b8 recreate_swapchain(renderer_backend* backend) {  // private function to recreate the swapchain, just takes in a pointer to the backend
    // if already being recreated, do not try again
    if (context.recreating_swapchain) {                                        // if the context is in a recreating swapchain stae
        KDEBUG("recreate_swapchain called when already recreating. Booting");  // throw a debug msg
        return false;                                                          // boot out
    }

    // detect if the window is too small to be drawn to such as minimized
    if (context.framebuffer_width == 0 || context.framebuffer_height == 0) {             // if either the width or the height of the framebuffer is 0
        KDEBUG("recreate_swapchain called when window is < 1 in a dimention. Booting");  // throe a debug msg
        return false;                                                                    // boot out
    }

    // mark as recreating swapchain
    context.recreating_swapchain = true;

    // wait for any operations to complete
    vkDeviceWaitIdle(context.device.logical_device);  // use the vulkan function to wait until the device is idle

    // clear these out just in case
    for (u32 i = 0; i < context.swapchain.image_count; ++i) {  // iterate through the swapchain images
        context.images_in_flight[i] = 0;                       // set each one to zero
    }

    // requery support -- because it may have changed since the last time we queried it
    vulkan_device_query_swapchain_support(               // call our function to query the swapchain support
        context.device.physical_device,                  // pass in the physical device
        context.surface,                                 // pass in the surface
        &context.device.swapchain_support);              // and an address to the swapchain support
    vulkan_device_detect_depth_format(&context.device);  // check the depth format of the device, pass in the address to the device - make sure that we have the most up to date format

    // recreate the swapchain, use our function
    vulkan_swapchain_recreate(
        &context,                   // pass in the address to the context
        cached_framebuffer_width,   // the cached frame buffer width
        cached_framebuffer_height,  // and height
        &context.swapchain);        // and the address to the swapchain being recreated

    // sync the framebuffer size with the cached sizes
    context.framebuffer_width = cached_framebuffer_width;
    context.framebuffer_height = cached_framebuffer_height;
    context.main_renderpass.render_area.z = context.framebuffer_width;
    context.main_renderpass.render_area.w = context.framebuffer_height;

    // reset the cached sizes to zero
    cached_framebuffer_width = 0;
    cached_framebuffer_height = 0;

    // syncronize the framebuffer size generation
    context.framebuffer_size_last_generation = context.framebuffer_size_generation;

    // cleanup the swapchain - free all of the command buffers
    for (u32 i = 0; i < context.swapchain.image_count; ++i) {                                                              // iterate through all of the swapchain images
        vulkan_command_buffer_free(&context, context.device.graphics_command_pool, &context.graphics_command_buffers[i]);  // send the graphics command buffer at index i, back to the graphics command pool. need to pass in the context
    }

    // destroy the framebuffers, they are no longer valid
    for (u32 i = 0; i < context.swapchain.image_count; ++i) {                                                       // iterate through the swapchain images
        vkDestroyFramebuffer(context.device.logical_device, context.world_framebuffers[i], context.allocator);      // use our function to destroy a framebuffer, pass in an address to the context, and the address to the frambuffer at index i, the one being destroyed
        vkDestroyFramebuffer(context.device.logical_device, context.swapchain.framebuffers[i], context.allocator);  // use our function to destroy a framebuffer, pass in an address to the context, and the address to the frambuffer at index i, the one being destroyed
    }

    // copy over a new render area - set the x and y to 0, and get the size from the framebuffer
    context.main_renderpass.render_area.x = 0;
    context.main_renderpass.render_area.y = 0;
    context.main_renderpass.render_area.z = context.framebuffer_width;
    context.main_renderpass.render_area.w = context.framebuffer_height;

    // re create the world framebuffers and the swapchain
    regenerate_framebuffers();  // call our regen framebuffers function, pass it the backend, an address to the swapchain, and an address to the main renderpass

    // re create the command buffers
    create_command_buffers(backend);  // pass it the backend

    // clear the recreating flag - no longer in recreating swapchain state
    context.recreating_swapchain = false;

    return true;
}

b8 create_buffers(vulkan_context* context) {
    VkMemoryPropertyFlagBits memory_property_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;  // only use memory that is local to the host, this makes it faster

    // geometry vertex buffer
    // set the vertex buffer size, after the vertex 3d is finished this will total 64mb
    const u64 vertex_buffer_size = sizeof(vertex_3d) * 1024 * 1024;
    if (!vulkan_buffer_create(   // call the create buffer function. and check if it succeeds
            context,             // pass it the context
            vertex_buffer_size,  // the size needed
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            memory_property_flags,              // pass in local memory bit
            true,                               // bind on creation
            &context->object_vertex_buffer)) {  // the address for where the buffer will be
        KERROR("Error creating vertex buffer.");
        return false;
    }
    context->geometry_index_offset = 0;

    // geometry index buffer
    // set the index buffer size,
    const u64 index_buffer_size = sizeof(u32) * 1024 * 1024;
    if (!vulkan_buffer_create(  // call the create buffer function. and check if it succeeds
            context,            // pass it the context
            index_buffer_size,  // the size needed
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            memory_property_flags,             // pass in local memory bit
            true,                              // bind on creation
            &context->object_index_buffer)) {  // the address for where the buffer will be
        KERROR("Error creating index buffer.");
        return false;
    }
    context->geometry_index_offset = 0;

    return true;
}

// create a texture, pass in a name, is it realeased automatically, the size, how many channels it hase,
// a pointer to the pixels in a u8 array, that is 8 bits per pixel, does it need transparency, and an address for the texture struct
void vulkan_renderer_create_texture(const u8* pixels, texture* texture) {
    // internal data creation
    // TODO: use an allocator for this - this will be done with a custom allocator as soon as we have the capability to
    texture->internal_data = (vulkan_texture_data*)kallocate(sizeof(vulkan_texture_data), MEMORY_TAG_TEXTURE);
    vulkan_texture_data* data = (vulkan_texture_data*)texture->internal_data;
    VkDeviceSize image_size = texture->width * texture->height * texture->channel_count;  // this is the number of pixels times the number of channels for those pixels - like the length of the pixels array

    // NOTE:  assumes there are 8 bits per channel - there is logic that can detect this, just not going to add it yet
    VkFormat image_format = VK_FORMAT_R8G8B8A8_UNORM;

    // create a staging buffer and load data into it - going to take the data in pixels and load it into vulkan memory
    VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;                                                           // this indicating that this is a transfer buffer
    VkMemoryPropertyFlags memory_prop_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;  // use both host visible and coherent memory
    vulkan_buffer staging;                                                                                                 // define the staging buffer
    vulkan_buffer_create(&context, image_size, usage, memory_prop_flags, true, &staging);                                  // create the buffer, give it the context, the image size, what its used for, memory flaga above, immediately bind, and the address for the buffer

    // load the data into the buffer, pass it the context, the buffer, starts at the beginning, size is the image size, no flags, and the data is pixels(the image)
    vulkan_buffer_load_data(&context, &staging, 0, image_size, 0, pixels);

    // create a vulkan image - will be making this more dynamic in the future
    vulkan_image_create(
        &context,          // pass in the context
        VK_IMAGE_TYPE_2D,  // is going to be a 2d image
        texture->width,    // pass in the size
        texture->height,
        image_format,                                                                                                                          // had coded to rgba 8 bit per channel for now
        VK_IMAGE_TILING_OPTIMAL,                                                                                                               // tiling is optimal
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,  // the image is both a transfer sourc and destination, it can be sampled, can be used for a color attachement
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,                                                                                                   // use local device memory faster
        true,                                                                                                                                  // create a view for the image
        VK_IMAGE_ASPECT_COLOR_BIT,                                                                                                             // its a color
        &data->image);                                                                                                                         // and the address to the image

    vulkan_command_buffer temp_buffer;                                                  // define a temprary command buffer
    VkCommandPool pool = context.device.graphics_command_pool;                          // define a graphics command pool
    VkQueue queue = context.device.graphics_queue;                                      // define a graphics queue
    vulkan_command_buffer_allocate_and_begin_single_use(&context, pool, &temp_buffer);  // allocate a command buffer for one use, use the pool and buffer defined above

    // transition the layout from whatever it is currently to optimal for recieving data
    vulkan_image_transition_layout(
        &context,                               // pass in the context
        &temp_buffer,                           // the command buffer just created
        &data->image,                           // address of the image being transitioned
        image_format,                           // hard coded to rgba 8 bits per channel for now
        VK_IMAGE_LAYOUT_UNDEFINED,              // old layout
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);  // new layout

    // copy the data from the buffer - copies the data stored in staging to the image
    vulkan_image_copy_from_buffer(&context, &data->image, staging.handle, &temp_buffer);

    // transition from optimal for data receipt to shader read only optimal layout
    vulkan_image_transition_layout(
        &context,
        &temp_buffer,
        &data->image,
        image_format,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,       // these two are only difference from previous call, old layout
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);  // new layout

    // end the single use command buffer
    vulkan_command_buffer_end_single_use(&context, pool, &temp_buffer, queue);

    // destoy the staging buffer, no longer needed
    vulkan_buffer_destroy(&context, &staging);

    // create a sampler for the texture
    // create the vulkan struct for creating a sampler, use the macro to format and fill with default values
    VkSamplerCreateInfo sampler_info = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    // TODO: these filters should be configurable
    sampler_info.magFilter = VK_FILTER_LINEAR;  // these are how it should be interpolated when it is magnified and such
    sampler_info.minFilter = VK_FILTER_LINEAR;
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;  // u, v, and w are used for texture coords
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;  // repeat means that the image will tile to fill the needed area
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.anisotropyEnable = VK_TRUE;  // is a type of texture filtering
    sampler_info.maxAnisotropy = 16;
    sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    sampler_info.unnormalizedCoordinates = VK_FALSE;
    sampler_info.compareEnable = VK_FALSE;
    sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;  // generates mip maps using linear interpolation
    sampler_info.mipLodBias = 0.0f;                           // but we arent using mip maps yet
    sampler_info.minLod = 0.0f;
    sampler_info.maxLod = 0.0f;

    // actually create the sampler, and check the results, pass it the logical device, the info created above, the memoty allocation stuffs, and an address for the sampler being created
    VkResult result = vkCreateSampler(context.device.logical_device, &sampler_info, context.allocator, &data->sampler);
    if (!vulkan_result_is_success(VK_SUCCESS)) {
        KERROR("Error creating texture sampler: %s", vulkan_result_string(result, true));
        return;
    }

    texture->generation++;  // incrememnt the generation - how many times this texture has been loaded, or refreshed
}

// destroy a texture
void vulkan_renderer_destroy_texture(texture* texture) {
    vkDeviceWaitIdle(context.device.logical_device);  // make sure that none of this stuff is in use before destoying anything

    vulkan_texture_data* data = (vulkan_texture_data*)texture->internal_data;  // conveneince pointer

    if (data) {
        vulkan_image_destroy(&context, &data->image);                                       // destoy the image
        kzero_memory(&data->image, sizeof(vulkan_image));                                   // zero out the memory from the image
        vkDestroySampler(context.device.logical_device, data->sampler, context.allocator);  // destroy the sampler
        data->sampler = 0;                                                                  // zero out the sampler

        kfree(texture->internal_data, sizeof(vulkan_texture_data), MEMORY_TAG_TEXTURE);  // free the kallocate memory for internal data
    }

    kzero_memory(texture, sizeof(struct texture));  // zero out the memory of the texture
}

// create a material
b8 vulkan_renderer_create_material(struct material* material) {
    if (material) {
        if (!vulkan_material_shader_acquire_resources(&context, &context.material_shader, material)) {
            KERROR("vulkan_renderer_create_material - failed to acquire shader resources.");
            return false;
        }

        KTRACE("Renderer: Material created.");
        return true;
    }

    KERROR("vulkan_renderer_create_material - called with null ptr. creation failed");
    return false;
}

// destroy a material
void vulkan_renderer_destroy_material(struct material* material) {
    if (material) {
        if (material->internal_id != INVALID_ID) {
            vulkan_material_shader_release_resources(&context, &context.material_shader, material);
        } else {
            KWARN("vulkan_renderer_destroy_material called with internal_id=INVALID_ID. nothing was done");
        }
    } else {
        KWARN("vulkan_renderer_destroy_material called with nullptr. nothing was done");
    }
}

// create geometry
b8 vulkan_renderer_create_geometry(geometry* geometry, u32 vertex_count, const vertex_3d* vertices, u32 index_count, const u32* indices) {
    if (!vertex_count || !vertices) {  // verify that vertex data has been passed in
        KERROR("vulkan_renderer_create_geometry requires vertex data, and none was supplied. vertex_count=%d, vertices=%p", vertex_count, vertices);
        return false;
    }

    // check if this is a re-upload. if it is, need to free old data afterward - determined re upload if the id is anything but invalid
    b8 is_reupload = geometry->internal_id != INVALID_ID;
    vulkan_geometry_data old_range;  // define a struct to store the old data

    vulkan_geometry_data* internal_data = 0;  // define a pointer to where the internal data is going to be stored
    if (is_reupload) {
        internal_data = &context.geometries[geometry->internal_id];  // set the data in internal data, with the contexts geometries at the index of the id

        // take a copy of the old range
        old_range.index_buffer_offset = internal_data->index_buffer_offset;
        old_range.index_count = internal_data->index_count;
        old_range.index_size = internal_data->index_size;
        old_range.vertex_buffer_offset = internal_data->vertex_buffer_offset;
        old_range.vertex_count = internal_data->vertex_count;
        old_range.vertex_size = internal_data->vertex_size;
    } else {
        for (u32 i = 0; i < VULKAN_MAX_GEOMETRY_COUNT; ++i) {
            if (context.geometries[i].id == INVALID_ID) {
                // found a free index
                geometry->internal_id = i;
                context.geometries[i].id = i;
                internal_data = &context.geometries[i];
                break;
            }
        }
    }

    if (!internal_data) {  // if no data has been stored in the internal data struct
        KFATAL("vulkan_renderer_create_geometry failed to find a free index for a new geometry upload.  Adjust the config to allow for more.");
        return false;
    }

    // grab a graphics command pool and the graphics queue
    VkCommandPool pool = context.device.graphics_command_pool;
    VkQueue queue = context.device.graphics_queue;

    // vertex data
    internal_data->vertex_buffer_offset = context.geometry_vertex_offset;
    internal_data->vertex_count = vertex_count;
    internal_data->vertex_size = sizeof(vertex_3d) * vertex_count;
    upload_data_range(&context, pool, 0, queue, &context.object_vertex_buffer, internal_data->vertex_buffer_offset, internal_data->vertex_size, vertices);
    // TODO: should maintain a free list instead of this
    context.geometry_vertex_offset += internal_data->vertex_size;

    // index data, if applicable
    if (index_count && indices) {
        internal_data->index_buffer_offset = context.geometry_index_offset;
        internal_data->index_count = index_count;
        internal_data->index_size = sizeof(u32) * index_count;
        upload_data_range(&context, pool, 0, queue, &context.object_index_buffer, internal_data->index_buffer_offset, internal_data->index_size, indices);
        // TODO: should maintain a free list instead of this
        context.geometry_index_offset += internal_data->index_size;
    }

    if (internal_data->generation == INVALID_ID) {
        internal_data->generation = 0;
    } else {
        internal_data->generation++;
    }

    if (is_reupload) {
        // free vertex data
        free_data_range(&context.object_vertex_buffer, old_range.vertex_buffer_offset, old_range.vertex_size);

        // free index data, if applicable
        if (old_range.index_size > 0) {
            free_data_range(&context.object_index_buffer, old_range.index_buffer_offset, old_range.index_size);
        }
    }

    // everything a success
    return true;
}

// destroy geometry
void vulkan_renderer_destroy_geometry(geometry* geometry) {
    if (geometry && geometry->internal_id != INVALID_ID) {
        vkDeviceWaitIdle(context.device.logical_device);
        vulkan_geometry_data* internal_data = &context.geometries[geometry->internal_id];

        // free the vertex data
        free_data_range(&context.object_vertex_buffer, internal_data->vertex_buffer_offset, internal_data->vertex_size);

        // free index data, if applicable
        if (internal_data->index_size > 0) {
            free_data_range(&context.object_index_buffer, internal_data->index_buffer_offset, internal_data->index_size);
        }

        // clean up data
        kzero_memory(internal_data, sizeof(vulkan_geometry_data));
        internal_data->id = INVALID_ID;
        internal_data->generation = INVALID_ID;
    }
}

// update an object using push constants, input a model to upload
void vulkan_renderer_draw_geometry(geometry_render_data data) {
    // ignore non uploaded geometries
    if (data.geometry && data.geometry->internal_id == INVALID_ID) {
        return;
    }

    // convenience pointers
    vulkan_geometry_data* buffer_data = &context.geometries[data.geometry->internal_id];
    vulkan_command_buffer* command_buffer = &context.graphics_command_buffers[context.image_index];

    // TODO: check if this is actually needed
    vulkan_material_shader_use(&context, &context.material_shader);

    // all we are doing for now is calling our shader set model, pass in the context the object shader, and the model to pass in
    vulkan_material_shader_set_model(&context, &context.material_shader, data.model);
    // if the geometry has a materidal apply it. if not use the default material
    material* m = 0;
    if (data.geometry->material) {
        m = data.geometry->material;
    } else {
        m = material_system_get_default();
    }
    vulkan_material_shader_apply_material(&context, &context.material_shader, m);

    // Bind vertex buffer at offset.
    VkDeviceSize offsets[1] = {buffer_data->vertex_buffer_offset};
    vkCmdBindVertexBuffers(command_buffer->handle, 0, 1, &context.object_vertex_buffer.handle, (VkDeviceSize*)offsets);

    // draw indexed or non-indexed.
    if (buffer_data->index_count > 0) {
        // Bind index buffer at offset.
        vkCmdBindIndexBuffer(command_buffer->handle, context.object_index_buffer.handle, buffer_data->index_buffer_offset, VK_INDEX_TYPE_UINT32);

        // Issue the draw.
        vkCmdDrawIndexed(command_buffer->handle, buffer_data->index_count, 1, 0, 0, 0);
    } else {
        vkCmdDraw(command_buffer->handle, buffer_data->vertex_count, 1, 0, 0);
    }
}