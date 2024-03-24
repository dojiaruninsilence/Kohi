#include "vulkan_renderpass.h"

#include "core/kmemory.h"

// create a vulkan renderpass
void vulkan_renderpass_create(
    vulkan_context* context,            // pass in a pointer to vulkan context
    vulkan_renderpass* out_renderpass,  // pass in a pointer to the vulkan renderpass being created
    f32 x, f32 y, f32 w, f32 h,         // pass in an x any y value, and a width and a height - this will be the render area - this and next line will be vec4s after we build our math library
    f32 r, f32 g, f32 b, f32 a,         // pass in rgba values - this will be the clear color
    f32 depth,                          // pass in a depth
    u32 stencil) {                      // pass in a stencil

    // copy over the render area info
    out_renderpass->x = x;
    out_renderpass->y = y;
    out_renderpass->w = w;
    out_renderpass->h = h;

    // copy over the rgba info
    out_renderpass->r = r;
    out_renderpass->g = g;
    out_renderpass->b = b;
    out_renderpass->a = a;

    // copy over the depth info
    out_renderpass->depth = depth;
    out_renderpass->stencil = stencil;

    // main subpass
    VkSubpassDescription subpass = {};                            // create the subpass with null value
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;  // set the subpass pipeline bind point to graphics - going to be used for graphics

    // attachments TODO: make this configurable
    const u32 attachment_description_count = 2;
    VkAttachmentDescription attachment_descriptions[attachment_description_count];  // create an array of attachment descriptions with attachement description count as the number of elements

    // color attachment
    VkAttachmentDescription color_attachment;                            // create color attachment description to fill out - another struct
    color_attachment.format = context->swapchain.image_format.format;    // TODO: configurable -- for now using the same as swapchain image format
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;                    // sampling count is set to 1 bit for now
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;               // load operations set to clear - contents of the render area will be cleared prior to renderpass - uses the access type
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;             // store operations - store the operations for future use
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;    // set to null - we are not using a stencil buffer
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;  // set to null - we are not using a stencil buffer
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;          // do not expect any particular layout before render pass starts
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;      // transitioned to after the render pass - refering to a format in memory
    color_attachment.flags = 0;                                          // not using any flags for now

    attachment_descriptions[0] = color_attachment;  // index 0 of the attachment description array is filled with the color attachment struct

    VkAttachmentReference color_attachment_reference;                              // create a vulkan color attachment reference struct
    color_attachment_reference.attachment = 0;                                     // attachment description array index -- the index of the array that we want to use
    color_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;  // set the reference layout to optimal

    subpass.colorAttachmentCount = 1;                         // set the subpass color attacment count to 1 - only have one color attachment
    subpass.pColorAttachments = &color_attachment_reference;  // pass the address to the color attachment reference into the subpass struct

    // depth attachment if there is one
    VkAttachmentDescription depth_attachment = {};                                    // create another acolor attachment description struct to fill out, this one is for the depth attachment
    depth_attachment.format = context->device.depth_format;                           // get the depth format from the device
    depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;                                 // sampling is set to 1 bit for now
    depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;                            // load operation set to clear
    depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;                      // store operation set to null
    depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;                 // stencil load operation set to null
    depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;               // dont expect any layout before the renderpass starts
    depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;                       // transitioned to after the render pass - refering to a format in memory
    depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;  // set the depth stencil attachment to optimal

    attachment_descriptions[1] = depth_attachment;  // index 1 of the attachment description array is filled with the depth attachment

    // depth attachment reference
    VkAttachmentReference depth_attachment_reference;                                      // create a vulkan depth attachment reference struct
    depth_attachment_reference.attachment = 1;                                             // attachment description array index - the index of the array we want to use
    depth_attachment_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;  // set the reference layout to optimal

    // TODO: other attachment types (input, resolve, preserve)

    // depth stencil data - tell the subpass about it using the address to the depth attachment reference
    subpass.pDepthStencilAttachment = &depth_attachment_reference;

    // input from a shader
    subpass.inputAttachmentCount = 0;  // this wil change once we start adding shaders
    subpass.pInputAttachments = 0;     // this wil change once we start adding shaders

    // attachments used for multisampling colour attachments
    subpass.pResolveAttachments = 0;

    // attachments not used in this subpass, but must be preserved for the next
    subpass.preserveAttachmentCount = 0;  // only have ones subpass at the moment
    subpass.pPreserveAttachments = 0;     // only have ones subpass at the moment

    // render pass dependencies. TODO: make this configurable
    VkSubpassDependency dependency;                                                                         // create a vulkan subpass dependency struct dependency
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;                                                            // when we have more than one subpasses, the source will go here. only have one for now
    dependency.dstSubpass = 0;                                                                              // destination, there isnt one, not using yet
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;                                // for stage mask using the colot attachment output bit
    dependency.srcAccessMask = 0;                                                                           // not using yet
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;                                // for stage mask using the colot attachment output bit
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;  // what memory access we want - either the color attachment write or read bits. - want both of them
    dependency.dependencyFlags = 0;                                                                         // not using yet

    // render pass create info
    VkRenderPassCreateInfo render_pass_create_info = {VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};  // create the struct for the vulcan renderpass creation info, using the vulcan macro as a template
    render_pass_create_info.attachmentCount = attachment_description_count;                        // created above
    render_pass_create_info.pAttachments = attachment_descriptions;                                // array crated above
    render_pass_create_info.subpassCount = 1;                                                      // only one subpass for now
    render_pass_create_info.pSubpasses = &subpass;                                                 // subpass created above
    render_pass_create_info.dependencyCount = 1;                                                   // hard coded for now
    render_pass_create_info.pDependencies = &dependency;                                           // vulkan struct created above
    render_pass_create_info.pNext = 0;                                                             // not using for now
    render_pass_create_info.flags = 0;                                                             // not using any flags for now

    VK_CHECK(vkCreateRenderPass(         // create the render pass and run against vk check
        context->device.logical_device,  // pass in the logical device
        &render_pass_create_info,        // pass in the info created above
        context->allocator,              // pass in the memory allocation stuffs
        &out_renderpass->handle));       // get the handle to the renderpass being created
}

// destroy a vulkan render pass, pass in a pointer to the vulkan context for the render pass and a pointer to the renderpass to be destroyed
void vulkan_renderpass_destroy(vulkan_context* context, vulkan_renderpass* renderpass) {
    if (renderpass && renderpass->handle) {                                                           // if there is a renderpass and the handle for it
        vkDestroyRenderPass(context->device.logical_device, renderpass->handle, context->allocator);  // pass both of them along with the physical device into vulkan destroy renderpass function
        renderpass->handle = 0;                                                                       // zero out the handle
    }
}

// begin a render pass
void vulkan_renderpass_begin(
    vulkan_command_buffer* command_buffer,  // pass in a vulkan command buffer
    vulkan_renderpass* renderpass,          // pass in the vulkan render pass to begin
    VkFramebuffer frame_buffer) {           // pass in a vulkan frame buffer

    // create the vulkan render pass begin struct - as with all of the others use the template proveded by vulkan
    VkRenderPassBeginInfo begin_info = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    begin_info.renderPass = renderpass->handle;           // give it the handle to the renderpass
    begin_info.framebuffer = frame_buffer;                // pass in the framebuffer
    begin_info.renderArea.offset.x = renderpass->x;       // pass in the x value of the render pass
    begin_info.renderArea.offset.y = renderpass->y;       // pass in the y value of the render pass
    begin_info.renderArea.extent.width = renderpass->w;   // pass in the width from the render pass
    begin_info.renderArea.extent.height = renderpass->h;  // pass in the height from the render pass

    VkClearValue clear_values[2];                                // one is for color and one is for depth.  - something to do with clearing the sceen?
    kzero_memory(clear_values, sizeof(VkClearValue) * 2);        // clear the entire block of memory to zeros, pass in the pointer to memory, size of a vulkan clear value times 2 because of the 2 values for color and depth
    clear_values[0].color.float32[0] = renderpass->r;            // pass in the render pass red value into color float index 0
    clear_values[0].color.float32[1] = renderpass->g;            // pass in the render pass green value into color float index 1
    clear_values[0].color.float32[2] = renderpass->b;            // pass in the render pass blue value into color float index 2
    clear_values[0].color.float32[3] = renderpass->a;            // pass in the render pass alpha value into color float index 3
    clear_values[1].depthStencil.depth = renderpass->depth;      // pass in the render pass depth value into the depth stencil depth
    clear_values[1].depthStencil.stencil = renderpass->stencil;  // pass in the render pass stencil value into the depth stencil stencil

    begin_info.clearValueCount = 2;          // set the clear value count to 2
    begin_info.pClearValues = clear_values;  // pass in the array we just created

    // issue first command
    vkCmdBeginRenderPass(command_buffer->handle, &begin_info, VK_SUBPASS_CONTENTS_INLINE);  // issue command to the command buffer attached to the provided handle, pass in the info just created, and a vulkan macro
    command_buffer->state = COMMAND_BUFFER_STATE_IN_RENDER_PASS;                            // set the command buffer state to in render pass
}

// end the vulkan render pass - pass in the vulkan command buffer and the vulkan renderpass to end -- this was the last line that i wrote - video is at 7 33
void vulkan_renderpass_end(vulkan_command_buffer* command_buffer, vulkan_renderpass* renderpass) {
    vkCmdEndRenderPass(command_buffer->handle);              // pass handle to command buffer to vulkan function to end the render pass
    command_buffer->state = COMMAND_BUFFER_STATE_RECORDING;  // set the command_buffer state to recording
}