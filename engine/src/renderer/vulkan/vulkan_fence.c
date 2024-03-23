#include "vulkan_fence.h"

#include "core/logger.h"

// create a vulkan fence
void vulkan_fence_create(
    vulkan_context* context,    // pass in a pointer to the vulkan context
    b8 create_signaled,         // if true will be created in a signaled state
    vulkan_fence* out_fence) {  // and a pointer to the fence being created

    // make sure to signal the fence if required
    out_fence->is_signaled = create_signaled;  // pass through whether its signaled
    // create a vulkan fence create info struct and use the provided macro to fill it out with default values
    VkFenceCreateInfo fence_create_info = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    if (out_fence->is_signaled) {                                // if the fence had been set to signaled
        fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;  // pass in a flag to create the fence signaled
    }

    // create the fence
    VK_CHECK(vkCreateFence(              // use the vulkan function to create a fence and check woth vk check
        context->device.logical_device,  // pass in the logical device
        &fence_create_info,              // and the info created above
        context->allocator,              // and the memory allocation stuffs
        &out_fence->handle));            // and the address to the handle of the fence being created
}

// destroy a vulkan fence - pass in a pointer to the context and the fence to be destroyed
void vulkan_fence_destroy(vulkan_context* context, vulkan_fence* fence) {
    if (fence->handle) {                     // if there is a handle to the fence being destroyed
        vkDestroyFence(                      // call the vulkan function to destroy the fence
            context->device.logical_device,  // pass in the logical device
            fence->handle,                   // and the handle to the fence being destroyed
            context->allocator);             // and the memory allocaton stuffs
    }
    fence->is_signaled = false;  // reset is signaled to false
}

// function to tell a fence to wait, takes in a pointer to the vulkan context, and a pointer to the fence to wait, and the time it should wait in nano seconds
b8 vulkan_fence_wait(vulkan_context* context, vulkan_fence* fence, u64 timeout_ns) {
    if (!fence->is_signaled) {               // if the fence is not signaled
        VkResult result = vkWaitForFences(   // call the vulkan function to wait for fences and soter the results in result
            context->device.logical_device,  // pass it the logical device
            1,                               // number of fences set to 0
            &fence->handle,                  // and address for the fence handle
            true,                            // wait all is set to true - wait on all fences passed in
            timeout_ns);                     // pass the time through
        switch (result) {                    // what was the result
            case VK_SUCCESS:                 // if success
                fence->is_signaled = true;   // fence is signaled
                return true;
            case VK_TIMEOUT:  // if not true, throw a warning and break out
                KWARN("vk_fence_wait - timed out");
                break;
            case VK_ERROR_DEVICE_LOST:
                KWARN("vk_fence_wait - VK_ERROR_DEVICE_LOST");
                break;
            case VK_ERROR_OUT_OF_HOST_MEMORY:
                KWARN("vk_fence_wait - VK_ERROR_OUT_OF_HOST_MEMORY");
                break;
            case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                KWARN("vk_fence_wait - VK_ERROR_OUT_OF_DEVICE_MEMORY");
                break;
            default:
                KERROR("vk_fence_wait - an unknown error has occured.");
                break;
        }
    } else {
        // if already signaled do not wait
        return true;
    }

    // did not work with a completely irregular way
    return false;
}

// reset a vulkan fence - takes in a pointer to the vulkan context, and a pointer to the fence being reset
void vulkan_fence_reset(vulkan_context* context, vulkan_fence* fence) {
    if (fence->is_signaled) {                                                        // if the fence is signaled
        VK_CHECK(vkResetFences(context->device.logical_device, 1, &fence->handle));  // run the vulkan function to reset fences, pass it the logical device, the number of fences ot reset and the handles to the fences to be reset. check with vk check
        fence->is_signaled = false;                                                  // fence is no longer signaled
    }
}