#include "vulkan_backend.h"

#include "vulkan_types.inl"
#include "vulkan_platform.h"
#include "vulkan_device.h"

#include "core/logger.h"
#include "core/kstring.h"

#include "containers/darray.h"

#include "platform/platform.h"

// create vulkan context - there will only be one
static vulkan_context context;

// foreward declaration
VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,    // takes in the severity of the message
    VkDebugUtilsMessageTypeFlagsEXT message_types,              // the type of message
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,  // the message
    void* user_data);                                           // could contain the name of the application or wherever it is running from

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
    KDEBUG("Vulkan surface created.")

    // vulkan device creation
    if (!vulkan_device_create(&context)) {
        KERROR("Failed to create device!");
        return FALSE;
    }

    KINFO("Vulkan renderer initialized successfully.");
    return TRUE;
}

void vulkan_renderer_backend_shutdown(renderer_backend* backend) {
    KDEBUG("Destroying Vulkan debugger...");
    if (context.debug_messenger) {                                                                                                                                   // check that it exists
        PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(context.instance, "vkDestroyDebugUtilsMessengerEXT");  // get a function pointer to destroy debug messenger ext func
        func(context.instance, context.debug_messenger, context.allocator);                                                                                          // then call the function
    }

    KDEBUG("Destroying Vulkan instance...");
    vkDestroyInstance(context.instance, context.allocator);  // use vulkan function to destroy the instance
}

void vulkan_renderer_backend_on_resized(renderer_backend* backend, u16 width, u16 height) {
}

b8 vulkan_renderer_backend_begin_frame(renderer_backend* backend, f32 delta_time) {
    return TRUE;
}

b8 vulkan_renderer_backend_end_frame(renderer_backend* backend, f32 delta_time) {
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