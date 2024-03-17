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
        return FALSE;
    }

    return TRUE;
}

void vulkan_device_destroy(vulkan_context* context) {
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

b8 select_physical_device(vulkan_context* context) {
    // query for what gpus are on the system
    u32 physical_device_count = 0;                                                       // count of devices on the system, initialized at zero
    VK_CHECK(vkEnumeratePhysicalDevices(context->instance, &physical_device_count, 0));  // get list of devices on the system, and check against vk success
    if (physical_device_count == 0) {                                                    // if there were no compatible devices found on the system
        KFATAL("No devices which support Vulkan were found.");
        return FALSE;
    }

    // if we do have devices retrieve those devices
    VkPhysicalDevice physical_devices[physical_device_count];                                           // create an arrary of physical devices, with the length of physical device count
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

        // TODO: these requirement should probably be driven by the engine
        // configuration -- if a device doesnt have all of these, it will move on to the next device
        vulkan_physical_device_requirements requirements = {};  // start with everything zeroed out
        requirements.graphics = TRUE;                           // graphics queue required
        requirements.present = TRUE;                            // presentation queue is required
        requirements.transfer = TRUE;                           // transfer queue is required
        // NOTE: enable this if compute will be required.
        // requirements.compute = TRUE;
        requirements.sampler_anisotropy = TRUE;                                              // support of sampler anisotropy is required
        requirements.discrete_gpu = TRUE;                                                    // discrete gpu is required
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
            break;
        }
    }

    // ensure that a device has been selected
    if (!context->device.physical_device) {  // if no devices were found
        KERROR("No physical devices wer found which meet the requirements.");
        return FALSE;
    }

    // founa a device
    KINFO("Physical device selected.");
    return TRUE;
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
            return FALSE;
        }
    }

    // obtain the queue family info from the device
    u32 queue_family_count = 0;                                                             // initialize queue family count with a value of 0
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, 0);               // get a count of the queue family properties
    VkQueueFamilyProperties queue_families[queue_family_count];                             // build an array to hold the family properties
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
            return FALSE;
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
                    b8 found = FALSE;                                                                                         // create found bool set to false
                    for (u32 j = 0; j < available_extension_count; ++j) {                                                     // iterate through the available extensions
                        if (strings_equal(requirements->device_extension_names[i], available_extensions[j].extensionName)) {  // if both of the strings(extension names) are the same
                            found = TRUE;                                                                                     // found it
                            break;                                                                                            // break out
                        }
                    }

                    if (!found) {  // if required device is not among the available devices
                        KINFO("Required extension not found: '%s', skipping device.", requirements->device_extension_names[i]);
                        kfree(available_extensions, sizeof(VkExtensionProperties) * available_extension_count, MEMORY_TAG_RENDERER);  // free the memory allocated to the extension count
                        return FALSE;
                    }
                }
            }

            kfree(available_extensions, sizeof(VkExtensionProperties) * available_extension_count, MEMORY_TAG_RENDERER);  // if everything is successful free the memory allocated to the extension count
        }

        // sampler anisotropy
        if (requirements->sampler_anisotropy && !features->samplerAnisotropy) {  // if anisotropy is required and the device doesnt have it, send message and boot out
            KINFO("Device does not support samplerAnisotropy, skipping.");
            return FALSE;
        }

        // device meets all requirements
        return TRUE;
    }

    // device failed
    return FALSE;
}