#include "vulkan_device.h"
#include "core/logger.h"
#include "core/kstring.h"
#include "core/kmemory.h"
#include "containers/darray.h"

// hold the data for the requirements for physical devices -- store this data for the devices being tested
typedef struct vulkan_physical_device_requirements {
    b8 graphics;  // does the device have a graphics queue -- for draw calls
    b8 present;   // does the device have a presentation queue -- presents to the screen
    b8 compute;   // does the device have a computation queue -- used for compute shaders
    b8 transfer;  // does device have a transfer queue -- transfer data from one format to another
    // darray
    const char** device_extension_names;  // point to a string then to a char, for a list of device extension names
    b8 sampler_anisotropy;                // does it support sampler anisotropy, need to look this up
    b8 discrete_gpu;                      // is it discreat or dedicated gpu, need to learn what this means
} vulkan_physical_device_requirements;

// hold all the info for the queue family -- the queue family indices
typedef struct vulkan_physical_device_queue_family_info {
    u32 graphics_family_index;
    u32 present_family_index;
    u32 compute_family_index;
    u32 transfer_family_index;
} vulkan_physical_device_queue_family_info;

// foreward declarations
b8 select_physical_device(vulkan_context* context);
b8 physical_device_meets_requirements(
    VkPhysicalDevice device,                                          // needs the physical device
    VkSurfaceKHR surface,                                             // needs the surface
    const VkPhysicalDeviceProperties* properties,                     // properties of the physical device
    const VkPhysicalDeviceFeatures* features,                         // features of the physical device
    const vulkan_physical_device_requirements* requirements,          // what is required of the physical device
    vulkan_physical_device_queue_family_info* out_queue_family_info,  // queue family struct
    vulkan_swapchain_support_info* out_swapchain_support);            // swapchain support info

b8 vulkan_device_create(vulkan_context* context) {
    // with the physical device, we arent actually creating a lphysical device, we are choosing one and getting all of its specs
    // there will be an algorithm that chooses the best gpu on the system, abased on several criteria
    // at first it will be simple and become more complex when we need it to be
    if (!select_physical_device(context)) {
        return false;
    }

    KINFO("Creating logical device...");
    // NOTE: for the time being we do not want to create additional queues for shared indices
    b8 present_shares_graphics_queue = context->device.graphics_queue_index == context->device.present_queue_index;    // if present shares the same queue as graphics, use the same queue
    b8 transfer_shares_graphics_queue = context->device.graphics_queue_index == context->device.transfer_queue_index;  // if transfer shares the same queue as graphics, use the same queue
    u32 index_count = 1;                                                                                               // keep track of the total indices, there has to be at least one
    if (!present_shares_graphics_queue) {                                                                              // if present has a different queue
        index_count++;                                                                                                 // inrcrement the index count
    }                                                                                                                  //
    if (!transfer_shares_graphics_queue) {                                                                             // if transfer has a different queue
        index_count++;                                                                                                 // inrcrement the index count
    }                                                                                                                  //
    u32 indices[32];                                                                                                   // create an array of indices the size of the max index count
    u8 index = 0;                                                                                                      // start at index zero
    indices[index++] = context->device.graphics_queue_index;                                                           // first element in the array will be the graphics queue index, increment index
    if (!present_shares_graphics_queue) {                                                                              // if present doesnt share the same queue as graphics
        indices[index++] = context->device.present_queue_index;                                                        // second element in the array will be the present queue index, increment index
    }                                                                                                                  //
    if (!transfer_shares_graphics_queue) {                                                                             // if transfer doesnt share the same queue as graphics
        indices[index++] = context->device.transfer_queue_index;                                                       // second or third element in array will be the transfer queue index, increment index
    }

    // need to look all of this stuff up
    VkDeviceQueueCreateInfo queue_create_infos[32];                                // vulkan function for creating an array of queue create information with an amount of the max index count
    for (u32 i = 0; i < index_count; ++i) {                                        // iterate through all of the queues and set the create infos
        queue_create_infos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;  // set the stype, he doesnt really talk about so look up
        queue_create_infos[i].queueFamilyIndex = indices[i];                       // set the queue family index to index i
        queue_create_infos[i].queueCount = 1;                                      // hard code for now may need to be more flexible in the future
        // TODO: enable this for future enhancement
        // if (indices[i] == context->device.graphics_queue_index) {                  // on the qraphics queue
        //     queue_create_infos[i].queueCount = 2;                                  // set the queue count to 2
        // }                                                                          //
        queue_create_infos[i].flags = 0;                           // zerod out for now
        queue_create_infos[i].pNext = 0;                           // zerod out for now
        f32 queue_priority = 1.0f;                                 // this is the default value for now
        queue_create_infos[i].pQueuePriorities = &queue_priority;  // set the queue priority here. will come back to this later
    }

    // request device feature
    // TODO: should be config driven
    VkPhysicalDeviceFeatures device_features = {};  // request for device features and set it empty
    device_features.samplerAnisotropy = VK_TRUE;    // request anisotropy

    // added per mac os support pull request, so not gonna get into, just want to keep in similar -----------------------------------------------------------
    b8 portability_required = false;
    u32 available_extension_count = 0;
    VkExtensionProperties* available_extensions = 0;
    VK_CHECK(vkEnumerateDeviceExtensionProperties(context->device.physical_device, 0, &available_extension_count, 0));
    if (available_extension_count != 0) {
        available_extensions = kallocate(sizeof(VkExtensionProperties) * available_extension_count, MEMORY_TAG_RENDERER);
        VK_CHECK(vkEnumerateDeviceExtensionProperties(context->device.physical_device, 0, &available_extension_count, available_extensions));
        for (u32 i = 0; i < available_extension_count; ++i) {
            if (strings_equal(available_extensions[i].extensionName, "VK_KHR_portability_subset")) {
                KINFO("Adding required extension 'VK_KHR_portability_subset'.");
                portability_required = true;
                break;
            }
        }
    }
    kfree(available_extensions, sizeof(VkExtensionProperties) * available_extension_count, MEMORY_TAG_RENDERER);

    u32 extension_count = portability_required ? 2 : 1;
    const char** extension_names = portability_required
                                       ? (const char* [2]){VK_KHR_SWAPCHAIN_EXTENSION_NAME, "VK_KHR_portability_subset"}
                                       : (const char* [1]){VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    // end of mac pull request -----------------------------------------------------------------------------------------------------------

    VkDeviceCreateInfo device_create_info = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};  // zero out device create info and pass in vulkan device create info
    device_create_info.queueCreateInfoCount = index_count;                           // set the queue create info count to the index count
    device_create_info.pQueueCreateInfos = queue_create_infos;                       // set the queue create infos
    device_create_info.pEnabledFeatures = &device_features;                          // set the enabled features to the device features
    /*device_create_info.enabledExtensionCount = 1;                                    // hard code the extension count to 1 for now
    const char* extensions_names = VK_KHR_SWAPCHAIN_EXTENSION_NAME;                  // the only extension we are going to load for now is the vk khr swapchain extension name
    device_create_info.ppEnabledExtensionNames = &extensions_names;                  // set the extension names */
    // // deleted per mac os support pull request --------------------------
    // added per mac os support pull request ------------------------------------------------------------------------------------------------
    device_create_info.enabledExtensionCount = extension_count;
    device_create_info.ppEnabledExtensionNames = extension_names;
    // end of mac pull request -----------------------------------------------------------------------------------------------------------

    // deprecated and ignored, so pass nothing
    device_create_info.enabledLayerCount = 0;
    device_create_info.ppEnabledLayerNames = 0;

    // create the device
    VK_CHECK(vkCreateDevice(                // call the create device and check against vh_check
        context->device.physical_device,    // pass in the vulkan physical device
        &device_create_info,                // pass in the device create info
        context->allocator,                 // pass in the memory allocation info
        &context->device.logical_device));  // and the logical device we are creating - a pointer we are storing for the device in vulkan device

    KINFO("Logical device created.");

    // queue arent created they are a part of the hardware -- this is us getting a handle to those so we can make use of them
    // get queues
    // graphics queue
    vkGetDeviceQueue(
        context->device.logical_device,        // pass in the logical device
        context->device.graphics_queue_index,  // pass in the graphics queue index
        0,                                     // hard coding the queue index to zero because graphics is the first in the index- probably going to change
        &context->device.graphics_queue);      // and a pointer to the graphics queue

    // present queue
    vkGetDeviceQueue(
        context->device.logical_device,       // pass in the logical device
        context->device.present_queue_index,  // pass in the present queue index
        0,                                    // hard coding the queue index to zero because graphics is the first in the index- probably going to change
        &context->device.present_queue);      // and a pointer to the present queue

    // transfer queue
    vkGetDeviceQueue(
        context->device.logical_device,        // pass in the logical device
        context->device.transfer_queue_index,  // pass in the transfer queue index
        0,                                     // hard coding the queue index to zero because graphics is the first in the index- probably going to change
        &context->device.transfer_queue);      // and a pointer to the transfer queue
    KINFO("Queues obtained.");

    // create a command pool for the graphics queue
    VkCommandPoolCreateInfo pool_create_info = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};  // create the vulkan command pool create info struct and use the provided function to fill fields with default info
    pool_create_info.queueFamilyIndex = context->device.graphics_queue_index;                 // pass graphics queue index into the queue family index - can only use it with this queue family, need to read more
    pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;                 // pass in the flag reset command buffer -- allows any command buffer allocated from it to be individually reset back to its initial state
    VK_CHECK(vkCreateCommandPool(                                                             // run the vulkan function to create a command pool against vk check
        context->device.logical_device,                                                       // pass it the logical device
        &pool_create_info,                                                                    // the info struct we just made
        context->allocator,                                                                   // the memory allocation stuffs
        &context->device.graphics_command_pool));                                             // and an address to the graphics command pool that we are creating

    // if the command pool is created successfully
    KINFO("Graphics command pool created.")

    return true;
}

void vulkan_device_destroy(vulkan_context* context) {
    // unset the queues - release the data for them
    context->device.graphics_queue = 0;
    context->device.present_queue = 0;
    context->device.transfer_queue = 0;

    // destroy the command pools
    KINFO("Destroying command pools...");
    vkDestroyCommandPool(                       // call the vulkan function to destroy command pool
        context->device.logical_device,         // pass in the logical device
        context->device.graphics_command_pool,  // the command pool to be destroyed
        context->allocator);                    // and the memory allocator stuffs

    // destroy the logical device
    KINFO("Destroying logical device...");
    if (context->device.logical_device) {                                     // if there is a logical device
        vkDestroyDevice(context->device.logical_device, context->allocator);  // use vulkan function to destroy the device, pass in the logical device and the memory allocation info
        context->device.logical_device = 0;                                   // set logical device to zero
    }
    // physical devices are not destroyed
    KINFO("Releasing physical device resources...");
    context->device.physical_device = 0;  // release the info for the physical device

    if (context->device.swapchain_support.formats) {                                      // if there is data in the swapchain support formats
        kfree(                                                                            // free it
            context->device.swapchain_support.formats,                                    // pass in the formats
            sizeof(VkSurfaceFormatKHR) * context->device.swapchain_support.format_count,  // and the size of the formats times the number of them
            MEMORY_TAG_RENDERER);                                                         // and the renederer memory tag
        context->device.swapchain_support.formats = 0;                                    // and reset the formats array
        context->device.swapchain_support.format_count = 0;                               // and the format count
    }

    // do the same thing for the present modes
    if (context->device.swapchain_support.present_modes) {                                    // if there is data in the swapchain support present modes
        kfree(                                                                                // free it
            context->device.swapchain_support.present_modes,                                  // pass in the present modes
            sizeof(VkPresentModeKHR) * context->device.swapchain_support.present_mode_count,  // and the size of the present modes times the number of them
            MEMORY_TAG_RENDERER);                                                             // and the renederer memory tag
        context->device.swapchain_support.present_modes = 0;                                  // and reset the present modes array
        context->device.swapchain_support.present_mode_count = 0;                             // and the present modes count
    }

    // reset the swapchain support data
    kzero_memory(
        &context->device.swapchain_support.capabilities,
        sizeof(context->device.swapchain_support.capabilities));

    // reset all of the queue family indices
    context->device.graphics_queue_index = -1;
    context->device.present_queue_index = -1;
    context->device.transfer_queue_index = -1;
}

void vulkan_device_query_swapchain_support(
    VkPhysicalDevice physical_device,                   // takes in the vulkan physical device
    VkSurfaceKHR surface,                               // takes in the vulkan surface
    vulkan_swapchain_support_info* out_support_info) {  // the support info to be queried

    // need to detect what the surface capabilities are
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(  // run the vulkan function to detect the surface capabilities of the device and run against vk check
        physical_device,                                 // pass in the vulkan physical device
        surface,                                         // pass in the vulkan surface
        &out_support_info->capabilities));               // and a poiter the info being queried

    // detect the surface formats
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(  // run the vulkan function to detect the surface formats of the device and run it against vk check
        physical_device,                            // pass in the vulkan physical device
        surface,                                    // pass in the vulkan surface
        &out_support_info->format_count,            // and a pointer to the format count
        0));

    if (out_support_info->format_count != 0) {                                                                                        // if format count isnt zero - there is something actually a format to create
        if (!out_support_info->formats) {                                                                                             // make sure that it hasnt already been created
            out_support_info->formats = kallocate(sizeof(VkSurfaceFormatKHR) * out_support_info->format_count, MEMORY_TAG_RENDERER);  // allocate memory for the format to be created
        }
        VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(  // get the formats, and check against vk_check
            physical_device,                            // pass in the vulkan physical device
            surface,                                    // pass in the vulkan surface
            &out_support_info->format_count,            // pass in a pointer to the format count
            out_support_info->formats));                // and the formats array to fill in
    }

    // detect the presentation modes
    VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(  // a vulkan function to get the present mode count and check against vk_check
        physical_device,                                 // pass in the vulkan physical device
        surface,                                         // pass in the vulkan surface
        &out_support_info->present_mode_count,           // and the present mode count to be filled in
        0));

    if (out_support_info->present_mode_count != 0) {                                                                                            // if the present mode count isnt zero - there is actually a mode to create
        if (!out_support_info->present_modes) {                                                                                                 // make sure the mode hasnt alreaky been created
            out_support_info->present_modes = kallocate(sizeof(VkPresentModeKHR) * out_support_info->present_mode_count, MEMORY_TAG_RENDERER);  // allocate memory for the mode to be created
        }
        VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(  // get the modes and check against vk_check
            physical_device,                                 // pass in the vulkan physical device
            surface,                                         // pass in the vulkan surface
            &out_support_info->present_mode_count,           // pass in a pointer to the present mode count
            out_support_info->present_modes));               // and the present modes array to fill in
    }
}

// used to get the depth format, pass in a vulkan device -- detects the image format that is required by our depth buffer - returns true if it is the correct format
b8 vulkan_device_detect_depth_format(vulkan_device* device) {
    // format candidates - determine the requirements
    const u64 candidate_count = 3;                           // set how many are available
    VkFormat candidates[3] = {                               // create an array of candidates, with higher want value at the top - in order of preferation
                              VK_FORMAT_D32_SFLOAT,          // one component, 32 bit signed floating point format that has 32 bit in the depth component
                              VK_FORMAT_D32_SFLOAT_S8_UINT,  // 2 component format, 32 signed float bits in the depth component, and 8 unsigned int bits in the stencil component, optionally 24 unused bits
                              VK_FORMAT_D24_UNORM_S8_UINT};  // 2 component format, 32 bit packed format, 8 unsigned int bits in stencil component, and 24 unsigned normalized bits in the depth component

    u32 flags = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;                                    // is a usage on how were going to use the image - depth and stencil buffers are combined
    for (u32 i = 0; i < candidate_count; ++i) {                                                    // iterate through the candidates
        VkFormatProperties properties;                                                             // create a vulkan format properties
        vkGetPhysicalDeviceFormatProperties(device->physical_device, candidates[i], &properties);  // use a function to get the device properties, pass in the physical device, canidates[i], and properties adress - check if it has the property we are looking for

        if ((properties.linearTilingFeatures & flags) == flags) {  // if it passes either of these checks, use this candidate for the depth buffer
            device->depth_format = candidates[i];
            return true;
        } else if ((properties.optimalTilingFeatures & flags) == flags) {
            device->depth_format = candidates[i];
            return true;
        }
    }

    // if it fails both checks
    return false;
}

b8 select_physical_device(vulkan_context* context) {
    // query for what gpus are on the system
    u32 physical_device_count = 0;                                                       // count of devices on the system, initialized at zero
    VK_CHECK(vkEnumeratePhysicalDevices(context->instance, &physical_device_count, 0));  // get list of devices on the system, and check against vk success
    if (physical_device_count == 0) {                                                    // if there were no compatible devices found on the system
        KFATAL("No devices which support Vulkan were found.");
        return false;
    }

    // if we do have devices retrieve those devices
    const u32 max_device_count = 32;                                                                    // create a variable to represent the maximum number of devices allowed
    VkPhysicalDevice physical_devices[max_device_count];                                                // create an arrary of physical devices, with the length of the max physical device count
    VK_CHECK(vkEnumeratePhysicalDevices(context->instance, &physical_device_count, physical_devices));  // this time instaed of getting a list, we fill the list into the physical devices array
    for (u32 i = 0; i < physical_device_count; ++i) {
        // use these to determine if the device meets the criteria that we need
        //  use vulkan functions to get the device properties of device[i] and store in properties
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(physical_devices[i], &properties);

        // use vulkan functions to get the device features of device[i] and store in features
        VkPhysicalDeviceFeatures features;
        vkGetPhysicalDeviceFeatures(physical_devices[i], &features);

        // use vulkan functions to get the device memory of device[i] and store in memory
        VkPhysicalDeviceMemoryProperties memory;
        vkGetPhysicalDeviceMemoryProperties(physical_devices[i], &memory);

        KINFO("Evaluating device: '%s', index %u.", properties.deviceName, i);

        // check if device supports local/host visible combo
        b8 supports_device_local_host_visible = false;
        for (u32 i = 0; i < memory.memoryTypeCount; ++i) {
            // check each memory type to see if its bit is set to 1
            if (
                ((memory.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0) &&
                ((memory.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != 0)) {
                supports_device_local_host_visible = true;
                break;
            }
        }

        // TODO: these requirement should probably be driven by the engine
        // configuration -- if a device doesnt have all of these, it will move on to the next device
        vulkan_physical_device_requirements requirements = {};  // start with everything zeroed out
        requirements.graphics = true;                           // graphics queue required
        requirements.present = true;                            // presentation queue is required
        requirements.transfer = true;                           // transfer queue is required
        // NOTE: enable this if compute will be required.
        // requirements.compute = true;
        requirements.sampler_anisotropy = true;  // support of sampler anisotropy is required
                                                 // added per mac os support pull request ------------------------------------------------------------------------------------------------

#if KPLATFORM_APPLE
        requirements.discrete_gpu = false;
#else
        // end of mac os support pull request ------------------------------------------------------------------------------------------------
        requirements.discrete_gpu = true;  // discrete gpu is required
#endif  // added per mac os support pull request

        requirements.device_extension_names = darray_create(const char*);                    // create an array to hold all the extension names
        darray_push(requirements.device_extension_names, &VK_KHR_SWAPCHAIN_EXTENSION_NAME);  // push required extenson names

        vulkan_physical_device_queue_family_info queue_info = {};  // initialize queue info zeroed out
        b8 result = physical_device_meets_requirements(            // check if the physical device meets the desired requirements and store result in result
            physical_devices[i],                                   // pass in the current physical device
            context->surface,                                      // pass in th vulkan surface
            &properties,                                           // pass in the devices properties
            &features,                                             // and the devices features
            &requirements,                                         // pass in the requirements
            &queue_info,                                           // pass in the queue family info
            &context->device.swapchain_support);                   // and the swapchain support info

        if (result) {                                                // if the physical device passed the requirements
            KINFO("Selected device: '%s'.", properties.deviceName);  // print out the info about the device
            // gpu type, etc.
            switch (properties.deviceType) {
                default:
                case VK_PHYSICAL_DEVICE_TYPE_OTHER:
                    KINFO("GPU type is Unknown.");
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                    KINFO("GPU type is Integrated.");
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                    KINFO("GPU type is Discrete.");
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                    KINFO("GPU type is Virtual.");
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_CPU:
                    KINFO("GPU type is CPU.");
                    break;
            }

            // print out the system driver information - using macros provided by vulkan
            KINFO(
                "GPU Driver API version: %d.%d.%d",
                VK_VERSION_MAJOR(properties.driverVersion),
                VK_VERSION_MINOR(properties.driverVersion),
                VK_VERSION_PATCH(properties.driverVersion));

            // print out the vulkan api version - using macros provided by vulkan
            KINFO(
                "Vulkan API version: %d.%d.%d",
                VK_VERSION_MAJOR(properties.apiVersion),
                VK_VERSION_MINOR(properties.apiVersion),
                VK_VERSION_PATCH(properties.apiVersion));

            // memory information
            for (u32 j = 0; j < memory.memoryHeapCount; ++j) {                                            // iterate through all the memory heaps
                f32 memory_size_gib = (((f32)memory.memoryHeaps[j].size) / 1024.0f / 1024.0f / 1024.0f);  // get the size of the memory heap [j] in GB
                if (memory.memoryHeaps[j].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {                      // if it is dedicated gpu memory
                    KINFO("Local GPU memory: %.2f GiB", memory_size_gib);
                } else {
                    KINFO("Shared System memory: %.2f GiB", memory_size_gib);
                }
            }

            // copy information about the physical device that we need to know
            context->device.physical_device = physical_devices[i];                    // pointer to the physical device
            context->device.graphics_queue_index = queue_info.graphics_family_index;  // get indices of the graphics queue
            context->device.present_queue_index = queue_info.present_family_index;    // get indices of the present queue
            context->device.transfer_queue_index = queue_info.transfer_family_index;  // get indices of the transfer queue
            // NOTE: set compute index here if needed

            // keep a copy of properties, features and memory info for later use
            context->device.properties = properties;  // get the properties of the physical device
            context->device.features = features;      // get the features of the physical device
            context->device.memory = memory;          // get the memory of the physical device
            context->device.supports_device_local_host_visible = supports_device_local_host_visible;
            break;
        }
    }

    // ensure that a device has been selected
    if (!context->device.physical_device) {  // if no devices were found
        KERROR("No physical devices wer found which meet the requirements.");
        return false;
    }

    // founa a device
    KINFO("Physical device selected.");
    return true;
}

b8 physical_device_meets_requirements(
    VkPhysicalDevice device,                                   // needs the physical device
    VkSurfaceKHR surface,                                      // needs the surface
    const VkPhysicalDeviceProperties* properties,              // properties of the physical device
    const VkPhysicalDeviceFeatures* features,                  // features of the physical device
    const vulkan_physical_device_requirements* requirements,   // what is required of the physical device
    vulkan_physical_device_queue_family_info* out_queue_info,  // queue family struct
    vulkan_swapchain_support_info* out_swapchain_support) {    // swapchain support info

    // evaluate device properties to determine if it meets the needs of out application
    // set all family queue info to -1 -- will actually wrap around to an insanely high number, which is ok
    out_queue_info->graphics_family_index = -1;
    out_queue_info->present_family_index = -1;
    out_queue_info->compute_family_index = -1;
    out_queue_info->transfer_family_index = -1;

    // is it a discrete gpu?
    if (requirements->discrete_gpu) {
        if (properties->deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            KINFO("Device is not a discrete GPU, and one is required. Skipping.");
            return false;
        }
    }

    // obtain the queue family info from the device
    u32 queue_family_count = 0;                                                             // initialize queue family count with a value of 0
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, 0);               // get a count of the queue family properties
    VkQueueFamilyProperties queue_families[32];                                             // build an array to hold the family properties - with the size of the maximum queue family count
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families);  // get the array of family properties

    // look at each queue and see what queues it supports
    KINFO("Graphics | Present | Compute | Transfer | Name");  // header row
    u8 min_transfer_score = 255;
    for (u32 i = 0; i < queue_family_count; ++i) {
        u8 current_transfer_score = 0;  // will be used to determine which queue will be used

        // does it have a graphics queue?
        if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            out_queue_info->graphics_family_index = i;  // graphics family index set to i
            ++current_transfer_score;                   // increment the transfer score
        }

        // does it have a compute queue?
        if (queue_families[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
            out_queue_info->compute_family_index = i;  // graphics family index set to i
            ++current_transfer_score;                  // increment the transfer score
        }

        // does it have a transfer queue
        if (queue_families[i].queueFlags & VK_QUEUE_TRANSFER_BIT) {
            // take the index if it is the current lowest. this increases the liklihood that it is a dedicated transfer queue
            if (current_transfer_score <= min_transfer_score) {  // if current transfer score lower than min transfer score
                min_transfer_score = current_transfer_score;     // then the current transfer score is the new min transfer score
                out_queue_info->transfer_family_index = i;       // transfer family index set to i
            }
        }

        // does it have a presentation queue
        VkBool32 supports_present = VK_FALSE;
        VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &supports_present));
        if (supports_present) {
            out_queue_info->present_family_index = i;
        }
    }

    // print out some info about the device
    KINFO("       %d |       %d |       %d |       %d | %s",
          out_queue_info->graphics_family_index != -1,
          out_queue_info->present_family_index != -1,
          out_queue_info->compute_family_index != -1,
          out_queue_info->transfer_family_index != -1,
          properties->deviceName);

    if (  // if the queue isnt a requirement, or it is requirement and there is actually an index for the queue - all four of these must be true
        (!requirements->graphics || (requirements->graphics && out_queue_info->graphics_family_index != -1)) &&
        (!requirements->present || (requirements->present && out_queue_info->present_family_index != -1)) &&
        (!requirements->compute || (requirements->compute && out_queue_info->compute_family_index != -1)) &&
        (!requirements->transfer || (requirements->transfer && out_queue_info->transfer_family_index != -1))) {
        KINFO("Device meets queue requirements.");
        KTRACE("Graphics Family Index: %i", out_queue_info->graphics_family_index);
        KTRACE("Present Family Index: %i", out_queue_info->present_family_index);
        KTRACE("Transfer Family Index: %i", out_queue_info->transfer_family_index);
        KTRACE("Compute Family Index: %i", out_queue_info->compute_family_index);

        // next we need to query swap chain support
        vulkan_device_query_swapchain_support(
            device,                  // pass in the vulkan device
            surface,                 // pass in the vulkan surface
            out_swapchain_support);  // and the swapchain support to be queried

        if (out_swapchain_support->format_count < 1 || out_swapchain_support->present_mode_count < 1) {                                                  // if there are no formats, or present modes
            if (out_swapchain_support->formats) {                                                                                                        // if there is anything stored here in formats
                kfree(out_swapchain_support->formats, sizeof(VkSurfaceFormatKHR) * out_swapchain_support->format_count, MEMORY_TAG_RENDERER);            // free it
            }                                                                                                                                            //
            if (out_swapchain_support->present_modes) {                                                                                                  // if there is anything stored in the present modes
                kfree(out_swapchain_support->present_modes, sizeof(VkPresentModeKHR) * out_swapchain_support->present_mode_count, MEMORY_TAG_RENDERER);  // free it
            }
            KINFO("Required swapchain support not present, skipping device.");
            return false;
        }

        // device extensions
        if (requirements->device_extension_names) {                                                                                // if there are required extensions
            u32 available_extension_count = 0;                                                                                     // initialize an extesion count at zero
            VkExtensionProperties* available_extensions = 0;                                                                       // prepare an array for the available extentions
            VK_CHECK(vkEnumerateDeviceExtensionProperties(                                                                         // get the count of available extensions
                device,                                                                                                            // pass in the vulkan device
                0,                                                                                                                 //
                &available_extension_count,                                                                                        // and the extenion count to fill in
                0));                                                                                                               //
            if (available_extension_count != 0) {                                                                                  // if there are extensions available
                available_extensions = kallocate(sizeof(VkExtensionProperties) * available_extension_count, MEMORY_TAG_RENDERER);  // allocate memory for an array using the size of vkextensionproperties times the count of them
                VK_CHECK(vkEnumerateDeviceExtensionProperties(                                                                     // get the available extentions and push into the array and check agains vk_check
                    device,                                                                                                        // pass in the device
                    0,                                                                                                             //
                    &available_extension_count,                                                                                    // pass in the count of extensions
                    available_extensions));                                                                                        // and the available extensions array to be filled in

                u32 required_extension_count = darray_length(requirements->device_extension_names);                           // initialize required extension count with the number of elements in the extension names as the value
                for (u32 i = 0; i < required_extension_count; ++i) {                                                          // iterate through all of the required extensions
                    b8 found = false;                                                                                         // create found bool set to false
                    for (u32 j = 0; j < available_extension_count; ++j) {                                                     // iterate through the available extensions
                        if (strings_equal(requirements->device_extension_names[i], available_extensions[j].extensionName)) {  // if both of the strings(extension names) are the same
                            found = true;                                                                                     // found it
                            break;                                                                                            // break out
                        }
                    }

                    if (!found) {  // if required device is not among the available devices
                        KINFO("Required extension not found: '%s', skipping device.", requirements->device_extension_names[i]);
                        kfree(available_extensions, sizeof(VkExtensionProperties) * available_extension_count, MEMORY_TAG_RENDERER);  // free the memory allocated to the extension count
                        return false;
                    }
                }
            }

            kfree(available_extensions, sizeof(VkExtensionProperties) * available_extension_count, MEMORY_TAG_RENDERER);  // if everything is successful free the memory allocated to the extension count
        }

        // sampler anisotropy
        if (requirements->sampler_anisotropy && !features->samplerAnisotropy) {  // if anisotropy is required and the device doesnt have it, send message and boot out
            KINFO("Device does not support samplerAnisotropy, skipping.");
            return false;
        }

        // device meets all requirements
        return true;
    }

    // device failed
    return false;
}