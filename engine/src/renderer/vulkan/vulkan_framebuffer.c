#include "vulkan_framebuffer.h"

#include "core/kmemory.h"

// create a vulkan framebuffer
void vulkan_framebuffer_create(
    vulkan_context* context,                // takes in a pointer to a vulkan context
    vulkan_renderpass* renderpass,          // a pointer to a vulkan renderpass
    u32 width,                              // a width
    u32 height,                             // a height
    u32 attachment_count,                   // the number of attachments
    VkImageView* attachments,               // a pointer to the array of attachments' views
    vulkan_framebuffer* out_framebuffer) {  // and a pointer to the vulkan framebuffer being created

    // take a copy of the attachments, renderpass and attachment count - in case they happen to change
    out_framebuffer->attachments = kallocate(sizeof(VkImageView) * attachment_count, MEMORY_TAG_RENDERER);  // allocate memory for an array of attachments, use the size of a vulkan image view times the number of attachments for the size
    for (u32 i = 0; i < attachment_count; ++i) {                                                            // iterate through the attachments
        out_framebuffer->attachments[i] = attachments[i];                                                   // push the attachments into the array
    }
    out_framebuffer->renderpass = renderpass;              // pass through
    out_framebuffer->attachment_count = attachment_count;  // pass through

    // create the vulkan frame buffer create info struct and use the provided macro to fill it with default values
    VkFramebufferCreateInfo framebuffer_create_info = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
    framebuffer_create_info.renderPass = renderpass->handle;              // pass it the handle to the renderpass
    framebuffer_create_info.attachmentCount = attachment_count;           // the attachment count saved from abov
    framebuffer_create_info.pAttachments = out_framebuffer->attachments;  // attachments from the ones saved above
    framebuffer_create_info.width = width;                                // pass through the width
    framebuffer_create_info.height = height;                              // pass through the height
    framebuffer_create_info.layers = 1;                                   // set the layers to one for now

    VK_CHECK(vkCreateFramebuffer(        // call the vulkan function to create a frambuffer and check with vk check
        context->device.logical_device,  // pass it the logical device
        &framebuffer_create_info,        // the info struct created above
        context->allocator,              // the memory allocation stuffs
        &out_framebuffer->handle));      // and the address to the handle of the framebuffer being created
}

// destroy a vulkan framebuffer
void vulkan_framebuffer_destroy(vulkan_context* context, vulkan_framebuffer* framebuffer) {                         // pass in a pointer to the vulkan context, and a pointer to the framebuffer being destroyed
    vkDestroyFramebuffer(context->device.logical_device, framebuffer->handle, context->allocator);                  // use the vulkan function to destroy a framebuffer, pass it the logical device, the handle to the framebuffer being destroyed, and the memory allocation stuffs
    if (framebuffer->attachments) {                                                                                 // if there are attachments in the array
        kfree(framebuffer->attachments, sizeof(VkImageView) * framebuffer->attachment_count, MEMORY_TAG_RENDERER);  // free the memory for the attachment array, pass it the array, uses the size of a vulkan image view times the number of attachments
    }
    // zero out the rest of the framebuffer struct
    framebuffer->handle = 0;
    framebuffer->attachment_count = 0;
    framebuffer->renderpass = 0;
}