#include "vulkan_backend.h"

#include "vulkan_types.inl"
#include "vulkan_platform.h"
#include "vulkan_device.h"
#include "vulkan_swapchain.h"
#include "vulkan_renderpass.h"
#include "vulkan_command_buffer.h"
#include "vulkan_framebuffer.h"
#include "vulkan_fence.h"
#include "vulkan_utils.h"

#include "core/logger.h"
#include "core/kstring.h"
#include "core/kmemory.h"
#include "core/application.h"

#include "containers/darray.h"

#include "platform/platform.h"

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

// foreward declaration
i32 find_memory_index(u32 type_filter, u32 property_flags);

void create_command_buffers(renderer_backend* backend);                                                               // private function to create command buffers, just takes in the renderer backend
void regenerate_framebuffers(renderer_backend* backend, vulkan_swapchain* swapchain, vulkan_renderpass* renderpass);  // private function to create/regenerate framebuffers, pass in pointers to the backend, the swapchain, and the renderpass - going to hook all these together
b8 recreate_swapchain(renderer_backend* backend);                                                                     // private function to recreate the swapchain, just takes in a pointer to the backend

b8 vulkan_renderer_backend_initialize(renderer_backend* backend, const char* application_name, struct platform_state* plat_state) {
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
        b8 found = FALSE;
        for (u32 j = 0; j < available_layer_count; ++j) {                                            // iterate through the available layers
            if (strings_equal(required_validation_layer_names[i], available_layers[j].layerName)) {  // if both are the same
                found = TRUE;
                KINFO("Found. ");
                break;
            }
        }

        if (!found) {  // if a required layer isnt in the list of available
            KFATAL("Required validation layer is missing: %s", required_validation_layer_names[i]);
            return FALSE;
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
    if (!platform_create_vulkan_surface(plat_state, &context)) {  // create surface, and check to see if it worked
        KERROR("Failed to create platform surface!");             // let us know if it failed
        return FALSE;
    }
    KDEBUG("Vulkan surface created.");

    // vulkan device creation
    if (!vulkan_device_create(&context)) {   // if it fails to create
        KERROR("Failed to create device!");  // trhow error
        return FALSE;                        // boot out
    }

    // vulkan swapchain creation
    vulkan_swapchain_create(
        &context,                    // pass in address to the context
        context.framebuffer_width,   // pass in a width
        context.framebuffer_height,  // pass in a height
        &context.swapchain);         // address to the swapchain being created

    // vulkan renderpass creation
    vulkan_renderpass_create(
        &context,                                                     // pass in address to context
        &context.main_renderpass,                                     // pass in the main renderpass
        0, 0, context.framebuffer_width, context.framebuffer_height,  // pass in an x, y, width and height - render area
        0.0f, 0.0f, 0.2f, 1.0f,                                       // pass in rgba values - clear color(dark blue)
        1.0f,                                                         // pass in a depth
        0);                                                           // pass in a stencil

    // create swapchain frame buffers
    context.swapchain.framebuffers = darray_reserve(vulkan_framebuffer, context.swapchain.image_count);  // create a swapchain framebuffer array and allocate memory for it using the size of a vulkan framebuffer times the swapchain image count
    regenerate_framebuffers(backend, &context.swapchain, &context.main_renderpass);                      // call our private function regenerate framebuffers, pass in the backend, and addresses to the swapchain, and the main renderpass

    // create command buffers
    create_command_buffers(backend);

    // create syncronization objects
    context.image_available_semaphores = darray_reserve(VkSemaphore, context.swapchain.max_frames_in_flight);  // create a darray and allocated memory for image available semaphores. use the size of a vulkan semaphore times the max frames in flight in the swapchain
    context.queue_complete_semaphores = darray_reserve(VkSemaphore, context.swapchain.max_frames_in_flight);   // create a darray and allocated memory for queue complete semaphores. use the size of a vulkan semaphore times the max frames in flight in the swapchain
    context.in_flight_fences = darray_reserve(vulkan_fence, context.swapchain.max_frames_in_flight);           // create a darray and allocated memory for in flight fences. use the size of a vulkan fence times the max frames in flight in the swapchain

    for (u8 i = 0; i < context.swapchain.max_frames_in_flight; ++i) {  // iterate through the max frames in flight
        // create a vulkan semaphore create info struct, and use the provided macro to input default values into the fields
        VkSemaphoreCreateInfo semaphore_create_info = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        // use the vulkan to create both image availabel semaphores and queue complete semaphores for image in flight at index i, pass in the logical device and the info created above, the memory allocation stuffs, and the address to the semaphore being created
        vkCreateSemaphore(context.device.logical_device, &semaphore_create_info, context.allocator, &context.image_available_semaphores[i]);
        vkCreateSemaphore(context.device.logical_device, &semaphore_create_info, context.allocator, &context.queue_complete_semaphores[i]);

        // create the fence in  a signaled state, indicating that the first frame has already been "rendered". this will prevent the application from waiting indefinately for the first frame to render
        // since it cannot be rendered until a frame is "rendered" before it
        vulkan_fence_create(&context, TRUE, &context.in_flight_fences[i]);  // use our function to create a fence, pass in the address to the context, set to create in signaled state, fence at index i to be created
    }

    // create the in flight images
    // in flight fences should not yet exist at this point, so clear the list. these are stored in pointers, because the initial state should be 0, and will be 0 when not in use
    // Actual fences are not owned by this list
    context.images_in_flight = darray_reserve(vulkan_fence, context.swapchain.image_count);  // create a darray and allocate memory for images in flight array, use the size of a vulkan fence times the number of images in the swap chain for the size
    for (u32 i = 0; i < context.swapchain.image_count; ++i) {                                // iterate through the swapchain images
        context.images_in_flight[i] = 0;                                                     // and make sure that all of the images in flight are zeroed out - nothing should be in flight jaust yet
    }

    // everything passed
    KINFO("Vulkan renderer initialized successfully.");
    return TRUE;
}

void vulkan_renderer_backend_shutdown(renderer_backend* backend) {
    // wait until the device is doing nothing at all
    vkDeviceWaitIdle(context.device.logical_device);

    // destroy in the opposite order of creation

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
        vulkan_fence_destroy(&context, &context.in_flight_fences[i]);  // use our function to destroy the fence at inde i, needs an address to the context, and an address to the fence being destroyed
    }
    darray_destroy(context.image_available_semaphores);  // destroy the image available semaphores darray and free the memory
    context.image_available_semaphores = 0;              // set the value of the destroyed darray to 0

    darray_destroy(context.queue_complete_semaphores);  // destroy the image available semaphores darray and free the memory
    context.queue_complete_semaphores = 0;              // set the value of the destroyed darray to 0

    darray_destroy(context.in_flight_fences);  // destroy the image available semaphores darray and free the memory
    context.in_flight_fences = 0;              // set the value of the destroyed darray to 0

    darray_destroy(context.images_in_flight);  // destroy the image available semaphores darray and free the memory
    context.images_in_flight = 0;              // set the value of the destroyed darray to 0

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
    for (u32 i = 0; i < context.swapchain.image_count; ++i) {                      // iterate through the swapchain images
        vulkan_framebuffer_destroy(&context, &context.swapchain.framebuffers[i]);  // use our function to destroy the framebuffer, pass in the address to the context, and the address to the frambuffer at index i
    }

    // destroy the render pass
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
    vulkan_device* device = &context.device;  // just to make writting the code a little bit easier and cleaner - conveneience pointer

    // check if recreating swap chain and boot out
    if (context.recreating_swapchain) {                                                                                           // is context in a recreating swapchain state
        VkResult result = vkDeviceWaitIdle(device->logical_device);                                                               // run the vulkan function to wait until the device is idle and store results in result
        if (!vulkan_result_is_success(result)) {                                                                                  // use function to determine the success of result, if failed
            KERROR("vulkan_renderer_backend_begin_frame vkDeviceWaitIdle (1) failed: '%s'", vulkan_result_string(result, TRUE));  // throw error using a string from result to fill out message
            return FALSE;                                                                                                         // boot out
        }
        // if waited successfully
        KINFO("Recreating swapchain, booting.");
        return FALSE;
    }

    // check if the framebuffer has been resized. if so, a new swapchain must be created
    if (context.framebuffer_size_generation != context.framebuffer_size_last_generation) {                                        // if the frambuffer size generation is not the same as last generation, the window has been resized
        VkResult result = vkDeviceWaitIdle(device->logical_device);                                                               // run the vulkan function to wait until the device is idle and store results in result
        if (!vulkan_result_is_success(result)) {                                                                                  // use function to determine the success of result, if failed
            KERROR("vulkan_renderer_backend_begin_frame vkDeviceWaitIdle (2) failed: '%s'", vulkan_result_string(result, TRUE));  // throw error using a string from result to fill out message
            return FALSE;                                                                                                         // boot out
        }

        // if the swapchain recreation failec(because, for example, the window was minimized), boot out before unsetting the flag
        if (!recreate_swapchain(backend)) {  // run the function to recreate the swapchain, and if it fails
            return FALSE;                    // boot out
        }

        KINFO("Resized, booting.");
        return FALSE;
    }

    // if no resizing and no swapchain recreation then
    // wait for the execution of the current frame to complete. the fence being free will allow this one to move on
    if (!vulkan_fence_wait(                                    // run the function and if it fails
            &context,                                          // pass in an address to the context
            &context.in_flight_fences[context.current_frame],  // get the in flight fence for the current frame
            UINT64_MAX)) {                                     // bogus value
        KWARN("in-flight fence wait failure");                 // throw warn if it fails
        return FALSE;                                          // boot out, not too bad an error, but dont continue
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
        return FALSE;
    }

    // begin recording commands
    vulkan_command_buffer* command_buffer = &context.graphics_command_buffers[context.image_index];  // convenience pointer, to svae time and cleaner code
    vulkan_command_buffer_reset(command_buffer);                                                     // reset the command buffer at image index to the ready state
    vulkan_command_buffer_begin(command_buffer, FALSE, FALSE, FALSE);                                // begin the command buffer with single use, renderpass continuos, and simultaneous use to false

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
    context.main_renderpass.w = context.framebuffer_width;
    context.main_renderpass.h = context.framebuffer_height;

    // begin the render pass
    vulkan_renderpass_begin(
        command_buffer,                                               // pass in the current command buffer
        &context.main_renderpass,                                     // an address to the main renderpass
        context.swapchain.framebuffers[context.image_index].handle);  // and the handle to the frame buffer at image index in context

    return TRUE;
}

b8 vulkan_renderer_backend_end_frame(renderer_backend* backend, f32 delta_time) {
    vulkan_command_buffer* command_buffer = &context.graphics_command_buffers[context.image_index];  // conveinience pointer

    // end the renderpass
    vulkan_renderpass_end(command_buffer, &context.main_renderpass);  // call our funtion to end renderpass, pass in the current graphics command buffer, and an address to the main renderpass

    // end the command buffer- calling our function. just pass in the current graphics command buffer
    vulkan_command_buffer_end(command_buffer);

    // make sure the previous frame is not using this image (i.e. its fence is being waited on)
    if (context.images_in_flight[context.image_index] != VK_NULL_HANDLE) {  // if there is acctually in images in flight at image index
        vulkan_fence_wait(                                                  // call our function to wait for a fence
            &context,                                                       // pass in an address to the context
            context.images_in_flight[context.image_index],                  // pass in the image in flight at the image index
            UINT64_MAX);                                                    // bogus high value
    }

    // mark the image fence as in use by this frame
    context.images_in_flight[context.image_index] = &context.in_flight_fences[context.current_frame];  // assign the current inflight fence to the image in flight at image index

    // reset the fence for use on the next frame
    vulkan_fence_reset(&context, &context.in_flight_fences[context.current_frame]);  // use out function to reset a fence, pass in the address to the context, the address to the in flight fence of the current frame

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

    VkResult result = vkQueueSubmit(                              // run the vulkan function to submit a queue and store the results in result
        context.device.graphics_queue,                            // pass in the graphics queue
        1,                                                        // only submitting one queue for now
        &submit_info,                                             // pass in the info struct we just created
        context.in_flight_fences[context.current_frame].handle);  // and the handle to the in flight fence at the index of the current frame

    // check the results of submit
    if (result != VK_SUCCESS) {                                                              // if the submit was unsuccessful
        KERROR("vkQueueSubmit failed with result: %s", vulkan_result_string(result, TRUE));  // throw out an error message, with the info of failure
        return FALSE;
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
    return TRUE;
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
            TRUE,                                                                           // set primary to true
            &context.graphics_command_buffers[i]);                                          // and and address to the command buffer being allocated
    }

    KINFO("Vulkan surface created.");
}

// private function to create/regenerate framebuffers, pass in pointers to the backend, the swapchain, and the renderpass - going to hook all these together
void regenerate_framebuffers(renderer_backend* backend, vulkan_swapchain* swapchain, vulkan_renderpass* renderpass) {
    for (u32 i = 0; i < swapchain->image_count; ++i) {  // iterate through all the swap chain image count. - we need a framebuffer for each swapchain image
        // TODO: make this dynamic based on the currently configured attachments
        u32 attachment_count = 2;                                        // set attachment count to 2, for now
        VkImageView attachments[] = {                                    // create a vulkan view attachments array
                                     swapchain->views[i],                // pass in the swap chain view at index i
                                     swapchain->depth_attachment.view};  // and the swapchain depth attachment view

        vulkan_framebuffer_create(                // call our function to create a framebuffer
            &context,                             // pass in an address to the context
            renderpass,                           // pass through the renderpass
            context.framebuffer_width,            // and the width
            context.framebuffer_height,           // and height
            attachment_count,                     // attachment counts defined above
            attachments,                          // array created above
            &context.swapchain.framebuffers[i]);  // address of the framebuffer being created at index i
    }
}

b8 recreate_swapchain(renderer_backend* backend) {  // private function to recreate the swapchain, just takes in a pointer to the backend
    // if already being recreated, do not try again
    if (context.recreating_swapchain) {                                        // if the context is in a recreating swapchain stae
        KDEBUG("recreate_swapchain called when already recreating. Booting");  // throw a debug msg
        return FALSE;                                                          // boot out
    }

    // detect if the window is too small to be drawn to such as minimized
    if (context.framebuffer_width == 0 || context.framebuffer_height == 0) {             // if either the width or the height of the framebuffer is 0
        KDEBUG("recreate_swapchain called when window is < 1 in a dimention. Booting");  // throe a debug msg
        return FALSE;                                                                    // boot out
    }

    // mark as recreating swapchain
    context.recreating_swapchain = TRUE;

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
    context.main_renderpass.w = context.framebuffer_width;
    context.main_renderpass.h = context.framebuffer_height;

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
    for (u32 i = 0; i < context.swapchain.image_count; ++i) {                      // iterate through the swapchain images
        vulkan_framebuffer_destroy(&context, &context.swapchain.framebuffers[i]);  // use our function to destroy a framebuffer, pass in an address to the context, and the address to the frambuffer at index i, the one being destroyed
    }

    // copy over a new render area - set the x and y to 0, and get the size from the framebuffer
    context.main_renderpass.x = 0;
    context.main_renderpass.y = 0;
    context.main_renderpass.w = context.framebuffer_width;
    context.main_renderpass.h = context.framebuffer_height;

    // re create the framebuffers
    regenerate_framebuffers(backend, &context.swapchain, &context.main_renderpass);  // call our regen framebuffers function, pass it the backend, an address to the swapchain, and an address to the main renderpass

    // re create the command buffers
    create_command_buffers(backend);  // pass it the backend

    // clear the recreating flag - no longer in recreating swapchain state
    context.recreating_swapchain = FALSE;

    return TRUE;
}