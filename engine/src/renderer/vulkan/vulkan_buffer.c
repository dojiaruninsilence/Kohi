#include "vulkan_buffer.h"

#include "vulkan_device.h"
#include "vulkan_command_buffer.h"
#include "vulkan_utils.h"

#include "core/logger.h"
#include "core/kmemory.h"

// create a vulkan buffer, pass in a pointer to the context, the size the buffer is going to need, what its usage is,
// and the type of memory needed, bool for bind on creation, and a pointer to where the resulting buffer will be held
b8 vulkan_buffer_create(
    vulkan_context* context,
    u64 size,
    VkBufferUsageFlagBits usage,
    u32 memory_property_flags,
    b8 bind_on_create,
    vulkan_buffer* out_buffer) {
    // zero out the memory for the resulting buffer and pass through values for the size, usage, and memory property flags
    kzero_memory(out_buffer, sizeof(vulkan_buffer));  // zero out the memory for where the buffer will be
    out_buffer->total_size = size;
    out_buffer->usage = usage;
    out_buffer->memory_property_flags = memory_property_flags;

    // create the vulkan struct for creating a buffer, use the macro to format and fill with default values
    VkBufferCreateInfo buffer_info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    buffer_info.size = size;                              // give it the size
    buffer_info.usage = usage;                            // and the usage
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;  // NOTE: only used in one queue

    // call vulkan function to create the buffer, give it the context, info above, memory allocation stuffs, and an address for the handle to the buffer being created
    VK_CHECK(vkCreateBuffer(context->device.logical_device, &buffer_info, context->allocator, &out_buffer->handle));

    // gather the memory requirements
    VkMemoryRequirements requirements;  // define a struct for the vulkan memory requirements
    // use the vulkan function to get the memory requirements, pass in the logical device, a handle to the buffer, and an address for the struct just defined
    // gets us how much memory, what type, ect
    vkGetBufferMemoryRequirements(context->device.logical_device, out_buffer->handle, &requirements);
    out_buffer->memory_index = context->find_memory_index(requirements.memoryTypeBits, out_buffer->memory_property_flags);  // get the memory index from the info provided above and the find memory index method
    if (out_buffer->memory_index == -1) {                                                                                   // if there is nothing in the memory index
        KERROR("Unable to create vulkan buffer because the required memory type index was not found.");                     // throw an error
        return false;                                                                                                       // boot out
    }

    // allocate memory info
    // create the vulkan struct for creating a buffer, use the macro to format and fill with default values
    VkMemoryAllocateInfo allocate_info = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocate_info.allocationSize = requirements.size;               // pass the size from the requirements struct created above
    allocate_info.memoryTypeIndex = (u32)out_buffer->memory_index;  // pass in the memory type

    // allocate the memory
    VkResult result = vkAllocateMemory(
        context->device.logical_device,  // pass it the logical device
        &allocate_info,                  // info from above
        context->allocator,              // the memory allocation infos
        &out_buffer->memory);            // and an address to the memory being allocated

    // check the results
    if (result != VK_SUCCESS) {                                                                                     // if fail
        KERROR("Unable to create vulkan buffer because the required memory allocation failed. Error: %i", result);  // throw error
        return false;                                                                                               // boot out
    }

    // if it passed
    if (bind_on_create) {                            // if bind on create set to true, then run the bind function
        vulkan_buffer_bind(context, out_buffer, 0);  // pass it the context, and the resulting buffer, and 0 for an offset
    }

    return true;
}

// destroy a vulkan buffer, just pass in the context and the buffer to destroy
void vulkan_buffer_destroy(vulkan_context* context, vulkan_buffer* buffer) {
    if (buffer->memory) {                                                                  // if there is memory to free
        vkFreeMemory(context->device.logical_device, buffer->memory, context->allocator);  // use vulkan function to free it
        buffer->memory = 0;                                                                // and reset memory
    }
    if (buffer->handle) {                                                                     // if there is a handle to a buffer
        vkDestroyBuffer(context->device.logical_device, buffer->handle, context->allocator);  // use vulkan function to destroy the buffer
        buffer->handle = 0;                                                                   // reset the handle
    }
    // reset buffer values
    buffer->total_size = 0;
    buffer->usage = 0;
    buffer->is_locked = false;
}

// resize the amount a buffer can hold - pass in a pointer to the context, the size that the context is being resized too,
// a pointer to the buffer being resized, a queue, and a command pool
b8 vulkan_buffer_resize(
    vulkan_context* context,
    u64 new_size,
    vulkan_buffer* buffer,
    VkQueue queue,
    VkCommandPool pool) {
    // create a new buffer
    // create the vulkan struct for creating a buffer, use the macro to format and fill with default values
    VkBufferCreateInfo buffer_info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    buffer_info.size = new_size;                          // give it the size
    buffer_info.usage = buffer->usage;                    // and the usage
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;  // NOTE: only used in one queue

    VkBuffer new_buffer;  // define a new vulkan buffer
    // call vulkan function to create the buffer, give it the context, info above, memory allocation stuffs, and an address for the handle to the buffer being created
    VK_CHECK(vkCreateBuffer(context->device.logical_device, &buffer_info, context->allocator, &new_buffer));

    // gather the memory requirements
    VkMemoryRequirements requirements;  // define a struct for the vulkan memory requirements
    // use the vulkan function to get the memory requirements, pass in the logical device, a handle to the buffer, and an address for the struct just defined
    // gets us how much memory, what type, ect
    vkGetBufferMemoryRequirements(context->device.logical_device, new_buffer, &requirements);

    // allocate memory info
    // create the vulkan struct for creating a buffer, use the macro to format and fill with default values
    VkMemoryAllocateInfo allocate_info = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocate_info.allocationSize = requirements.size;           // pass the size from the requirements struct created above
    allocate_info.memoryTypeIndex = (u32)buffer->memory_index;  // pass in the memory type

    // allocate the memory
    VkDeviceMemory new_memory;
    VkResult result = vkAllocateMemory(
        context->device.logical_device,  // pass it the logical device
        &allocate_info,                  // info from above
        context->allocator,              // the memory allocation infos
        &new_memory);                    // and an address to the memory being allocated

    // bind the new buffer's memory - pass it the logical device, the new buffer, new memory, and a zero offset
    VK_CHECK(vkBindBufferMemory(context->device.logical_device, new_buffer, new_memory, 0));

    // copy over the data using our function
    vulkan_buffer_copy_to(context, pool, 0, queue, buffer->handle, 0, new_buffer, 0, buffer->total_size);

    // make sure that anything that might beusing that buffer is finished
    vkDeviceWaitIdle(context->device.logical_device);

    // destroy the old
    if (buffer->memory) {                                                                  // if there is memory to free
        vkFreeMemory(context->device.logical_device, buffer->memory, context->allocator);  // use vulkan function to free it
        buffer->memory = 0;                                                                // and reset memory
    }
    if (buffer->handle) {                                                                     // if there is a handle to a buffer
        vkDestroyBuffer(context->device.logical_device, buffer->handle, context->allocator);  // use vulkan function to destroy the buffer
        buffer->handle = 0;                                                                   // reset the handle
    }

    // set new properties
    buffer->total_size = new_size;
    buffer->memory = new_memory;
    buffer->handle = new_buffer;

    return true;
}

// bind a vulkan buffer - they cannot be used if they are not bound, pass in pointers to the context, the buffer being bound, and the offset of the buffer
void vulkan_buffer_bind(vulkan_context* context, vulkan_buffer* buffer, u64 offset) {
    // bind the new buffer's memory - pass it the logical device, the buffer handle, buffer memory, and pass in the offset
    VK_CHECK(vkBindBufferMemory(context->device.logical_device, buffer->handle, buffer->memory, offset));
}

// these 2 functions surround the loading of the data - they do opposite functions - lock maps the data in the buffer, and unlock unmaps it
void* vulkan_buffer_lock_memory(vulkan_context* context, vulkan_buffer* buffer, u64 offset, u64 size, u32 flags) {
    // use the vulkan function map memory , to map all the data in the buffer to data void pointer
    void* data;
    VK_CHECK(vkMapMemory(context->device.logical_device, buffer->memory, offset, size, flags, &data));
    return data;
}
void vulkan_buffer_unlock_memory(vulkan_context* context, vulkan_buffer* buffer) {
    vkUnmapMemory(context->device.logical_device, buffer->memory);
}

// load data into a vulkan buffer -- pass in a pointers to the context and the buffer to
// load data into, pass in the offset, the size, and the flags. then the data to load
void vulkan_buffer_load_data(vulkan_context* context, vulkan_buffer* buffer, u64 offset, u64 size, u32 flags, const void* data) {
    void* data_ptr;                                                                                         // define a place to put some data
    VK_CHECK(vkMapMemory(context->device.logical_device, buffer->memory, offset, size, flags, &data_ptr));  // map the memory
    kcopy_memory(data_ptr, data, size);                                                                     // copy in the new data
    vkUnmapMemory(context->device.logical_device, buffer->memory);                                          // unmap the data
}

// copy data from one buffer to another, pass in a pointer to the context, a command pool, a fence, a queue,
// the buffer being copied, how big the source buffer is, and the offset of the source buffer , the destination buffer, and its offset
void vulkan_buffer_copy_to(
    vulkan_context* context,
    VkCommandPool pool,
    VkFence fence,
    VkQueue queue,
    VkBuffer source,
    u64 source_offset,
    VkBuffer dest,
    u64 dest_offset,
    u64 size) {
    vkQueueWaitIdle(queue);  // wait until the queue passed in is finished with anything it is doing
    // create a one-time use command buffer
    vulkan_command_buffer temp_command_buffer;                                                 // define a temporary command buffer
    vulkan_command_buffer_allocate_and_begin_single_use(context, pool, &temp_command_buffer);  // create single use cmd buffer, pass in the context, the pool to pull from, and the address for the cmd buffer

    // prepare the copy command and add it to the command buffer
    VkBufferCopy copy_region;
    copy_region.srcOffset = source_offset;
    copy_region.dstOffset = dest_offset;
    copy_region.size = size;

    vkCmdCopyBuffer(temp_command_buffer.handle, source, dest, 1, &copy_region);  // use the vulkan function to copy the buffer, give it the command buffer, the source buffer, and the resulting buffer, only one region and the region

    // Submit the buffer for execution and wait for it to complete.
    vulkan_command_buffer_end_single_use(context, pool, &temp_command_buffer, queue);
}