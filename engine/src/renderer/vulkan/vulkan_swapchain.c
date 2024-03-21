#include "vulkan_swapchain.h"

#include "core/logger.h"
#include "core/kmemory.h"
#include "vulkan_device.h"
#include "vulkan_image.h"

// foreward declarations
//  some internal functions to call rather than reapeating code
void create(vulkan_context* context, u32 width, u32 height, vulkan_swapchain* swapchain);  // take in a pointer to the context, the size(which will be the window size to start), and a pointer to the vulkan swapchain stuct
void destroy(vulkan_context* context, vulkan_swapchain* swapchain);                        // pass in a pointer to the context, and a pointer to the vulkan swapchain struct

// create the vulkan swap chain
void vulkan_swapchain_create(
    vulkan_context* context,            // pass in a pointer to the vulkan context
    u32 width,                          // pass in a width
    u32 height,                         // pass in a height
    vulkan_swapchain* out_swapchain) {  // pass in a pointer to the swap chain struct
    // simply create a new one
    create(context, width, height, out_swapchain);  // call the internal create function
}

// recreate the vulkan swapchain
void vulkan_swapchain_recreate(
    vulkan_context* context,        // pass in a pointer to the vulkan context
    u32 width,                      // pass in a width
    u32 height,                     // pass in a height
    vulkan_swapchain* swapchain) {  // pass in a pointer to the vulkan swapchain struct
    // destroy the old and create a new one
    destroy(context, swapchain);                // call the internal destroy function
    create(context, width, height, swapchain);  // call the internal create function
}

// destroy the vulkan swapchain
void vulkan_swapchain_destroy(
    vulkan_context* context,        // pass in a pointer to the context
    vulkan_swapchain* swapchain) {  // pass in a pointer to the vulkan swapchain struct
    destroy(context, swapchain);    // call the internal destroy function
}

// get the next image in the swapchain to be used
b8 vulkan_swapchain_acquire_next_image_index(
    vulkan_context* context,                // pass in the context
    vulkan_swapchain* swapchain,            // pass in the swapchain struct
    u64 timeout_ns,                         // pass in a timeout value, in nano secs
    VkSemaphore image_available_semaphore,  // pass in a semaphore - which is a way to sync up gpu operations with other gpu operations
    VkFence fence,                          // pass in a fence - which is used to sync up operations between the application and the gpu
    u32* out_image_index) {                 // pointer to the out img index we wanted

    // use a vulkan function to get the next image in the swapchain, track the results in result
    VkResult result = vkAcquireNextImageKHR(
        context->device.logical_device,  // pass in the logical device
        swapchain->handle,               // pass in the handle to the swapchain
        timeout_ns,                      // pass in a timeout value in nano seconds
        image_available_semaphore,       // pass in a semaphore - a way to sync up gpu to gpu operations
        fence,                           // pass in a fence - a way for the application and gpu operations to sync up
        out_image_index);                // get the next image index

    // analyze the result
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {  // if there was an error for out of date khr -- basically means that the swapchain needs to recreated
        // trigger swapchain recreation, then boot out of the render loop
        vulkan_swapchain_recreate(context, context->framebuffer_width, context->framebuffer_height, swapchain);  // swapchain recreate, pass in context, context framebuffer for the width and height, and the swap chain
        return FALSE;                                                                                            // boot out
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {                                            // if result wasnt successful and wasnt suboptimal khr either
        KFATAL("Failed to acquire swapchain image!");                                                            // fatal message
        return FALSE;                                                                                            // boot out
    }

    // if result was a succes
    return TRUE;
}

// provides the presentation of the rendered to image - returns the image back to the swap chain after being rendered to
void vulkan_swapchain_present(
    vulkan_context* context,                // pass in a pointer to the context
    vulkan_swapchain* swapchain,            // pass in a pointer to the swapchain struct
    VkQueue graphics_queue,                 // pass in the handle to the graphics queue
    VkQueue present_queue,                  // pass in the handle to the present queue
    VkSemaphore render_complete_semaphore,  // pass in a semaphore - for syncronization between the app and the gpu -- needed for queued operations
    u32 present_image_index) {              // pass in the present image index

    // return the image to the swapchain fpr presentation
    VkPresentInfoKHR present_info = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};  // take in the structure type
    present_info.waitSemaphoreCount = 1;                                   // one semaphore to wait for while it completes operations
    present_info.pWaitSemaphores = &render_complete_semaphore;             // expects an array of semaphores, we only have one for now, so pass in the address to it
    present_info.swapchainCount = 1;                                       // can have multiple swapchains but we oly have one for now
    present_info.pSwapchains = &swapchain->handle;                         // take in the handle to the swapchain(s) - again probably expecting an array
    present_info.pImageIndices = &present_image_index;                     // take in the present image inices again this is an array, but we ony have one right now. this is what is returned from aquire next image index func above
    present_info.pResults = 0;                                             // not using for now so set to null

    VkResult result = vkQueuePresentKHR(present_queue, &present_info);        // the vulkan function to return the image to the swapcahin for presentation,  takes in a handle to the present queue, and the present info we just created, results stored in result
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {  // if result is out of date, or is suboptimal
        // swapchain is out of date, suboptimal, or a framebuffer resize has occured. trigger swapchain recreation
        vulkan_swapchain_recreate(context, context->framebuffer_width, context->framebuffer_height, swapchain);  // call vulkan swapchain recreate function, pass in the context, the widt and height of the framebuff, and the swapchain struct
    } else if (result != VK_SUCCESS) {                                                                           // if it isnt successful
        KFATAL("Failed to present swap chain image!");                                                           // throw fatal error
    }

    // increment and loop the index
    context->current_frame = (context->current_frame + 1) % swapchain->max_frames_in_flight;  // increment the current frame and if that number is higher than the max frames in flight, then loop it back around to 1
}

void create(vulkan_context* context, u32 width, u32 height, vulkan_swapchain* swapchain) {  // take in a pointer to the context, the size(which will be the window size to start), and a pointer to the vulkan swapchain stuct
    VkExtent2D swapchain_extent = {width, height};                                          // info the swapchain needs when it is being created, takes in a width and a height
    swapchain->max_frames_in_flight = 2;                                                    // may lower this if we need to, this means that we will be triple buffering. we will be able to render to 2 frames while a 3rd is being drawn. get higher framerates and such

    // choose a swap surface format
    b8 found = FALSE;                                                              // create found set to false
    for (u32 i = 0; i < context->device.swapchain_support.format_count; ++i) {     // iterate through all of the swapchain formats
        VkSurfaceFormatKHR format = context->device.swapchain_support.formats[i];  // create vulkan surface format set to the value of swapchain formats[i]
        // preferred formats
        if (format.format == VK_FORMAT_B8G8R8A8_UNORM &&               // if the format contains format b8g8r8a8 unorm - meaning its bgra and 8 bits per channel
            format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {  // and color space rgb nonlinear - uses the color space srgb non linear
            swapchain->image_format = format;                          // set swapchain image format to format
            found = TRUE;                                              // found is now true
            break;
        }
    }

    // if there is no preferred format, pick the first one that is listed
    if (!found) {
        swapchain->image_format = context->device.swapchain_support.formats[0];
    }

    VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;                         // create present mode and set it to fifo khr, this is only one that will for sure exist, so set to default. it has to exist(fifo = first in first out) - good for movies, one frame after another, can be slower and less responsive
    for (u32 i = 0; i < context->device.swapchain_support.present_mode_count; ++i) {  // iterate through the swapchain's present mode count
        VkPresentModeKHR mode = context->device.swapchain_support.present_modes[i];   // create a new mode and set to swapchain present modes[i]
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {                                    // if mode equals mailbox khr- instead of using the first frame uses the most current frame that exists and throw out the other ones, use triple buffering by default
            present_mode = mode;                                                      // replace fifo with mailbox for the present mode
            break;
        }
    }

    // for good measure - requery the swapchain support
    vulkan_device_query_swapchain_support(
        context->device.physical_device,      // pass in the vulkan physical device
        context->surface,                     // pass in the vulkan surface
        &context->device.swapchain_support);  // and a pointer to the swapchain support to be queried

    // if whatever was passed into the extent isnt valid, replace it with something that is
    // swapchain extent - need to take a look at the capapbilities of the extent of the swapchain - i am super confused about some of this, and uint32 max could be broken
    if (context->device.swapchain_support.capabilities.currentExtent.width != UINT32_MAX) {  // if extent has an actual value
        swapchain_extent = context->device.swapchain_support.capabilities.currentExtent;     // store it in swapchain_extent
    }

    // clamp to the value allowed by the GPU
    VkExtent2D min = context->device.swapchain_support.capabilities.minImageExtent;     // get min from minimageextent
    VkExtent2D max = context->device.swapchain_support.capabilities.maxImageExtent;     // get max from maximeageextent
    swapchain_extent.width = KCLAMP(swapchain_extent.width, min.width, max.width);      // clamp the swap chain extent width to the max and min provided
    swapchain_extent.height = KCLAMP(swapchain_extent.height, min.height, max.height);  // clamp the swap chain extent height to the max and min provided

    // -- this will be the number of images that we are going to use
    u32 image_count = context->device.swapchain_support.capabilities.minImageCount + 1;  // get image count by getting how many images the swapchain is capable of and adding one - 99% is going to be 2
    // this sets the image count to whatever the maximun count that the device allows
    if (context->device.swapchain_support.capabilities.maxImageCount > 0 && image_count > context->device.swapchain_support.capabilities.maxImageCount) {
        image_count = context->device.swapchain_support.capabilities.maxImageCount;
    }

    // swapchain create info
    VkSwapchainCreateInfoKHR swapchain_create_info = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};  // create the struct swapchain create info to store the info, and start with the vulkan swapchain cerat info
    swapchain_create_info.surface = context->surface;                                                // define the surface from the context's surface
    swapchain_create_info.minImageCount = image_count;                                               // define the min image count using image count
    swapchain_create_info.imageFormat = swapchain->image_format.format;                              // define the image format using the image format stored in swapchain struct
    swapchain_create_info.imageColorSpace = swapchain->image_format.colorSpace;                      // define the image color space using the color space stored in the swapchain format
    swapchain_create_info.imageExtent = swapchain_extent;                                            // define the image extent using the swapchain extent
    swapchain_create_info.imageArrayLayers = 1;                                                      // hard code the image array layers to 1
    swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;                          // use the vulkan macro color attachment bit ti define image usage -- very important. the same as the color cuffer in open gl

    // setup the queue family indices
    if (context->device.graphics_queue_index != context->device.present_queue_index) {  // if the graphics and present dont share a common queue
        u32 queueFamilyIndices[] = {                                                    // create an array of queue family indices
                                    (u32)context->device.graphics_queue_index,          // pass in the graphics queue index
                                    (u32)context->device.present_queue_index};          // and the present queue index
        swapchain_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;            // define the image sharing mode to concurrent
        swapchain_create_info.queueFamilyIndexCount = 2;                                // define the index count at 2
        swapchain_create_info.pQueueFamilyIndices = queueFamilyIndices;                 // define the queue family indices using the array created
    } else {                                                                            // if they share the same queue
        swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;             // define the image sharing mode as exclusive
        swapchain_create_info.queueFamilyIndexCount = 0;                                // define the queue family index count as 0
        swapchain_create_info.pQueueFamilyIndices = 0;                                  // and the indices at 0 as well
    }

    // final bit of the create info
    swapchain_create_info.preTransform = context->device.swapchain_support.capabilities.currentTransform;  // define the pretransform using swapchain cappabilities currenttransform - transform of image vs presentation - like portrait vs landscape as an example
    swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;                              // define the composite alpha to opaque bit -- the surface will be opaque always
    swapchain_create_info.presentMode = present_mode;                                                      // define present mode using whats stored in present mode
    swapchain_create_info.clipped = VK_TRUE;                                                               // define clipped as true
    swapchain_create_info.oldSwapchain = 0;                                                                // define the old swapchain as null

    // this is the actual create call function
    // check the reults against vk check, take in the logical device, an address to the swapchain create info we just filled out, the memory allocator, and the address for the swapchain handle
    VK_CHECK(vkCreateSwapchainKHR(context->device.logical_device, &swapchain_create_info, context->allocator, &swapchain->handle));

    // start with a zero frame index
    context->current_frame = 0;

    // images - start off with zero images
    swapchain->image_count = 0;
    // retrieve the count of the images first
    //  pass in the logical device, the handle to the swapchain, and the swapchain image count and check against VK_Check
    VK_CHECK(vkGetSwapchainImagesKHR(context->device.logical_device, swapchain->handle, &swapchain->image_count, 0));
    if (!swapchain->images) {                                                                                           // if the swapchain images are empty and the field is null
        swapchain->images = (VkImage*)kallocate(sizeof(VkImage) * swapchain->image_count, MEMORY_TAG_RENDERER);         // allocate memory for the images, use the size of a vulkan image times the count, and use the renderer tag
    }                                                                                                                   //
    if (!swapchain->views) {                                                                                            // if the swapchain views are empty and the field is null
        swapchain->views = (VkImageView*)kallocate(sizeof(VkImageView) * swapchain->image_count, MEMORY_TAG_RENDERER);  // allocate memory for the images, use the size of a vulkan image view times the count, and use the renderer tag
    }
    // call for the second time, but this time pass in the images to be created using the memory just allocated
    VK_CHECK(vkGetSwapchainImagesKHR(context->device.logical_device, swapchain->handle, &swapchain->image_count, swapchain->images));

    // views - the swap chain creates the images for us but not the views, so we do that here
    for (u32 i = 0; i < swapchain->image_count; ++i) {                                 // iterate through all the images
        VkImageViewCreateInfo view_info = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};  // create a struct for the view create info called view info with the provided vulkan structure
        view_info.image = swapchain->images[i];                                        // define image with image stored in swapchain
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;                                    // define vulkan view type as 2d
        view_info.format = swapchain->image_format.format;                             // define format with format stored in swapchain
        view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;             // define aspect mask to color bit -- again this is the color buffer
        view_info.subresourceRange.baseMipLevel = 0;                                   // base level of mip mapping?
        view_info.subresourceRange.levelCount = 1;                                     // only going to have one level of mip mapping, need to look these up
        view_info.subresourceRange.baseArrayLayer = 0;                                 // base level of array layer
        view_info.subresourceRange.layerCount = 1;                                     // only going to have one array layer

        // call the vulkan function to create the views, pass in the logical device, the view info that we just filled out, the memory allocator, and the address for the swapchai view at index i
        VK_CHECK(vkCreateImageView(context->device.logical_device, &view_info, context->allocator, &swapchain->views[i]));
    }

    // need to create an image resource for the depth buffer
    // depth resources
    if (!vulkan_device_detect_depth_format(&context->device)) {  // run the function to detect the dpeth format, if it has the correct format then set the depth format to it if it doesnt match any of the candidates then
        context->device.depth_format = VK_FORMAT_UNDEFINED;      // set the depth format to undefined
        KFATAL("Failed to find a supported format!");            // and throw a fatal error
    }

    // Create depth image and its view
    vulkan_image_create(
        context,                                      // pass in the context
        VK_IMAGE_TYPE_2D,                             // image type is 2d
        swapchain_extent.width,                       // pass in extent width
        swapchain_extent.height,                      // pass in extent height
        context->device.depth_format,                 // pass in the depth format from the device
        VK_IMAGE_TILING_OPTIMAL,                      // let gpu decide tiling
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,  // the usage
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,          // want to use gpu memory
        TRUE,                                         // create a view
        VK_IMAGE_ASPECT_DEPTH_BIT,                    // use this for the depth buffer
        &swapchain->depth_attachment);                // where all this is stored

    KINFO("Swapchain created successfully");
}

void destroy(vulkan_context* context, vulkan_swapchain* swapchain) {  // pass in a pointer to the context, and a pointer to the vulkan swapchain struct
    vkDeviceWaitIdle(context->device.logical_device);                 // use the vulkan function to wait until the device is idle
    vulkan_image_destroy(context, &swapchain->depth_attachment);      // destroy the depth attachment

    // only detroy the views, not the images, since those are owned by the swapchain and are thus destroyed when it is
    for (u32 i = 0; i < swapchain->image_count; ++i) {                                                // iterate through the views
        vkDestroyImageView(context->device.logical_device, swapchain->views[i], context->allocator);  // and destroy them
    }

    vkDestroySwapchainKHR(context->device.logical_device, swapchain->handle, context->allocator);  // destroy the swapchain
}
