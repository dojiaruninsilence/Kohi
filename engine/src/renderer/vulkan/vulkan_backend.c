#include "vulkan_backend.h"

#include "vulkan_types.inl"

#include "core/logger.h"

// create vulkan context - there will only be one
static vulkan_context context;

b8 vulkan_renderer_backend_initialize(renderer_backend* backend, const char* application_name, struct platform_state* plat_state) {
    // TODO: custom allocator
    context.allocator = 0;  // zero out the custom allocator

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
    // will be going over these in the next video, but they do need to be zeroed out for now
    create_info.enabledExtensionCount = 0;
    create_info.ppEnabledExtensionNames = 0;
    create_info.enabledLayerCount = 0;
    create_info.ppEnabledLayerNames = 0;

    // call the actual creation method
    VkResult result = vkCreateInstance(&create_info, context.allocator, &context.instance);  // pass in the info created above, the custom allocator(default for now), and the address to where the context handle is being stored.  returns a vkresult, so we atroe that in result
    if (result != VK_SUCCESS) {                                                              // if creation wasnt successful
        KERROR("vkCreateInstance failed with result: %u", result);
        return FALSE;
    }

    KINFO("Vulkan renderer initialized successfully.");
    return TRUE;
}

void vulkan_renderer_backend_shutdown(renderer_backend* backend) {
}

void vulkan_renderer_backend_on_resized(renderer_backend* backend, u16 width, u16 height) {
}

b8 vulkan_renderer_backend_begin_frame(renderer_backend* backend, f32 delta_time) {
    return TRUE;
}

b8 vulkan_renderer_backend_end_frame(renderer_backend* backend, f32 delta_time) {
    return TRUE;
}