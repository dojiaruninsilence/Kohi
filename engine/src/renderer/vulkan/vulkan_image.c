#include "vulkan_image.h"

#include "vulkan_device.h"

#include "core/kmemory.h"
#include "core/logger.h"

void vulkan_image_create(
    vulkan_context* context,               // pass in a ponter to the vulkan context
    VkImageType image_type,                // pass in the type of image to create
    u32 width,                             // pass in a width
    u32 height,                            // pass in a height
    VkFormat format,                       // pass in a vulkan format
    VkImageTiling tiling,                  //
    VkImageUsageFlags usage,               // if there are any that we want or need to pass in
    VkMemoryPropertyFlags memory_flags,    // if there are any that we need/want to pass in
    b32 create_view,                       // specify whether a view should be created for this image or not
    VkImageAspectFlags view_aspect_flags,  // if there are any that we want/need to pass in
    vulkan_image* out_image) {             // the vulcan struct that is created

    // copy over the width and the height parameters
    out_image->width = width;
    out_image->height = height;

    // here is the create info structure
    VkImageCreateInfo image_create_info = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};  // create the structure, with the provided vulkan structure
    image_create_info.imageType = VK_IMAGE_TYPE_2D;                               // set image type to 2d - hard coded for now, may change- probably use a 3d image struct
    image_create_info.extent.width = width;                                       // set the extent width
    image_create_info.extent.height = height;                                     // set the extent height
    image_create_info.extent.depth = 1;                                           // TODO: support configurable depth-- only need a s depth of 1 while it is a 2d image
    image_create_info.mipLevels = 4;                                              // TODO: support mip mapping
    image_create_info.arrayLayers = 1;                                            // TODO: support number of layers in the image
    image_create_info.format = format;                                            // pass through
    image_create_info.tiling = tiling;                                            // pass through - generally want to use optimal, other is linear. allows the driver to decide itself
    image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;                  // not transitioning this image from a nother memory layout
    image_create_info.usage = usage;                                              // pass through
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;                            // TODO: configurable sample count -- single sampling for now
    image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;                    // TODO: configurable sharing mode

    // run the vulkan function to create an image, pass in the logical device, the create info we jsust filled in, the memory allocatot, and a handle to the image that is creates, run against vk_check
    VK_CHECK(vkCreateImage(context->device.logical_device, &image_create_info, context->allocator, &out_image->handle));

    // query the memory requirements - using a vulkan function
    VkMemoryRequirements memory_requirements;                                                               // struct to be filled in
    vkGetImageMemoryRequirements(context->device.logical_device, out_image->handle, &memory_requirements);  // fill in the struct, give it the logical device and the handle to the image being queried

    i32 memory_type = context->find_memory_index(memory_requirements.memoryTypeBits, memory_flags);  // check the device to see where it stores the neccassary memory type
    if (memory_type == -1) {                                                                         // if it comes back with nothing
        KERROR("Required memory type not found. Image not valid");                                   // throw an error
    }

    // allocate the memory
    // VkMemoryAllocateInfo memory_allocate_info = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO};  // create a memory allocate info stuct, using the vulkan macro to set it up -- changed from his code per the documentation <-------------------------
    VkMemoryAllocateInfo memory_allocate_info = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};  // create a memory allocate info stuct, using the vulkan macro to set it up
    memory_allocate_info.allocationSize = memory_requirements.size;                        // allocation size set to the requires size
    memory_allocate_info.memoryTypeIndex = memory_type;                                    // memory type index set to memory type

    // use a vulkan funtion to allocate the memory to the device, pass in the logical device, and the info we just filled in, the memory allovator, and the address to the memory being allocated out
    VK_CHECK(vkAllocateMemory(context->device.logical_device, &memory_allocate_info, context->allocator, &out_image->memory));

    // bind the memory using a vulkan function, pass the logical device, a handle to the image being created, the memory allocated for the image and a memory offset(used for image pooling) hard codded to 0 for now
    VK_CHECK(vkBindImageMemory(context->device.logical_device, out_image->handle, out_image->memory, 0));  // TODO: configurable memory offset

    // create view
    if (create_view) {                                                            // if set to true in info
        out_image->view = 0;                                                      // clear view to 0
        vulkan_image_view_create(context, format, out_image, view_aspect_flags);  // call vulkan image view create function and pass in the context, format, image assosiated with it, and any view aspect flags
    }
}

void vulkan_image_view_create(
    vulkan_context* context,            // pass in a pointer to the vulkan context
    VkFormat format,                    // pass in the vulkan format
    vulkan_image* image,                // pass in the vulcan image the view is being created for
    VkImageAspectFlags aspect_flags) {  // any aspect flags that we may or may not need
    VkImageViewCreateInfo view_create_info = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    view_create_info.image = image->handle;                       // use the image handle for the image
    view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;            // TODO: make configurable
    view_create_info.format = format;                             // pass through
    view_create_info.subresourceRange.aspectMask = aspect_flags;  // pass in any needed or wanted

    // TODO: MAKE CONFIGURABLE - both set to a single layer for now
    view_create_info.subresourceRange.baseMipLevel = 0;
    view_create_info.subresourceRange.levelCount = 1;
    view_create_info.subresourceRange.baseArrayLayer = 0;
    view_create_info.subresourceRange.layerCount = 1;

    // use the vulkan function to create an image view, pass in the logical device, the info just created, memory allocator, and an address to the view
    VK_CHECK(vkCreateImageView(context->device.logical_device, &view_create_info, context->allocator, &image->view));
}

// transitions the provided image from old_layout to new_layout- convert from an image the disk understands to an image the gpu understands
// pass it pointers to the context, a command buffer, and a vulkan image, also pass it the format, and the old and new layouts
void vulkan_image_transition_layout(
    vulkan_context* context,
    vulkan_command_buffer* command_buffer,
    vulkan_image* image,
    VkFormat format,
    VkImageLayout old_layout,
    VkImageLayout new_layout) {
    // the memory barrier ensures that any commands issued before the barrier is created use the old format, and any after use the new one
    // syncroniztion tool - he didnt go into a ton of detail here, if we dont come back to this then i need to look it up
    // create the vulkan struct for creating an image memory barrier, use the macro to format and fill with default values
    VkImageMemoryBarrier barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    barrier.oldLayout = old_layout;  // pass through the old and new layouts
    barrier.newLayout = new_layout;
    barrier.srcQueueFamilyIndex = context->device.graphics_queue_index;  // use the graphics queue index for both the source and destination queue families
    barrier.dstQueueFamilyIndex = context->device.graphics_queue_index;
    barrier.image = image->handle;  // pass in the handle to the image
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags source_stage;
    VkPipelineStageFlags dest_stage;

    // dont care about the old layout - transition to optimal layout (for the underlying implementation).
    if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        // dont care what stage the pipeline is in at the start
        source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

        // used for copying
        dest_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        // transitioning fram a transfer destination layout to a shader-readonly layout
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        // from a copying stage to...
        source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;

        // the fragment stage
        dest_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else {
        KFATAL("unsupported layout transition!");
        return;
    }

    // this is what actually performs the layout transition
    vkCmdPipelineBarrier(
        command_buffer->handle,
        source_stage, dest_stage,
        0,
        0, 0,
        0, 0,
        1, &barrier);
}

// copies data in buffer to provided image
// @param context the vulkan context
// @param image the image to copy the buffer's data to
// @param the buffer whose data will be copied
void vulkan_image_copy_from_buffer(
    vulkan_context* context,
    vulkan_image* image,
    VkBuffer buffer,
    vulkan_command_buffer* command_buffer) {
    // region to copy
    VkBufferImageCopy region;                          // create a buffer image copy struct
    kzero_memory(&region, sizeof(VkBufferImageCopy));  // zero out the memory of the stuct
    // not using these, just here to remember that they exist
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;        // not using yet, have to change a bunch before this first
    region.imageSubresource.baseArrayLayer = 0;  // not using layers
    region.imageSubresource.layerCount = 1;      // so only the one

    region.imageExtent.width = image->width;  // get the width and height from the image
    region.imageExtent.height = image->height;
    region.imageExtent.depth = 1;  // these are only 2d images here - need more stuff to work with 3d images

    vkCmdCopyBufferToImage(
        command_buffer->handle,
        buffer,
        image->handle,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region);
}

void vulkan_image_destroy(vulkan_context* context, vulkan_image* image) {
    if (image->view) {                                                                        // if there is a view
        vkDestroyImageView(context->device.logical_device, image->view, context->allocator);  // destroy it
        image->view = 0;
    }
    if (image->memory) {                                                                  // if memory is allocated
        vkFreeMemory(context->device.logical_device, image->memory, context->allocator);  // free it
        image->memory = 0;
    }
    if (image->handle) {                                                                    // if there is an image
        vkDestroyImage(context->device.logical_device, image->handle, context->allocator);  // destroy it
        image->handle = 0;
    }
}