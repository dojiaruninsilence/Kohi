#include "vulkan_command_buffer.h"

#include "core/kmemory.h"

void vulkan_command_buffer_allocate(
    vulkan_context* context,                      // takes in a pointer to the vulkan context
    VkCommandPool pool,                           // the pool the command buffer will be coming from
    b8 is_primary,                                // primary if set to true. secondary buffers can ne used inside of another command buffer
    vulkan_command_buffer* out_command_buffer) {  // a pointer to the command buffer being taken

    kzero_memory(out_command_buffer, sizeof(out_command_buffer));  // zero out the memory allocated to the out command buffer - just to make sure that there had been nothing left behind in this structure

    // create the vulkan command buffer allocate info stuct and use the provided vulkan template for it
    VkCommandBufferAllocateInfo allocate_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    allocate_info.commandPool = pool;                                                                        // pass in the pool
    allocate_info.level = is_primary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;  // if is primary is true set the level to primary, if false set to secondary(can be used in another command buffer)
    allocate_info.commandBufferCount = 1;                                                                    // allocating a single command buffer at a time. this may change
    allocate_info.pNext = 0;                                                                                 // not using this for now, so set to zero

    out_command_buffer->state = COMMAND_BUFFER_STATE_NOT_ALLOCATED;  // set to its default state
    VK_CHECK(vkAllocateCommandBuffers(                               // run the vulkan function to allocate a command buffer against the vk check -- this can allocate multiple command buffers
        context->device.logical_device,                              // pass it the logical device
        &allocate_info,                                              // and the info that was just created
        &out_command_buffer->handle));                               // and the address to the handle(double pointer) to the buffer being allocated
    // if this passed, then filp the command buffer state
    out_command_buffer->state = COMMAND_BUFFER_STATE_READY;  // is now ready to begin issuing commands
}

// vulkan command buffer free -- since its an allocation we free rather than destroy - this would be the putting it back into the command pool
void vulkan_command_buffer_free(
    vulkan_context* context,                  // take in a pointer to the vulkan context
    VkCommandPool pool,                       // the pool that the command buffer came from and will return too
    vulkan_command_buffer* command_buffer) {  // the pointer to the command buffer being freed
    vkFreeCommandBuffers(                     // call the vulkan function to free the command buffers
        context->device.logical_device,       // pass it the logical device
        pool,                                 // and the pool the command buffer will return to
        1,                                    // the number of command buffers is set to one
        &command_buffer->handle);             // and give it the address to the handle of the command buffer being returned to the pool

    command_buffer->handle = 0;                                  // zero out the command buffer handle
    command_buffer->state = COMMAND_BUFFER_STATE_NOT_ALLOCATED;  // set the command buffer back to its default state
}

// vulkan command buffer begin -- this is the begin render recording state
void vulkan_command_buffer_begin(
    vulkan_command_buffer* command_buffer,  // pass a pointer to the command buffer to begin
    b8 is_single_use,                       // is it only going to be used once and will be reset again and recorded again between each submission
    b8 is_renderpass_continue,              // is going to be used continuously - says that a secondary command buffer will exist entirely within a render pass, if is a primary buffer this will be ignored
    b8 is_simultaneous_use) {               // is it going to be used simultaneously -- that it can be resubmitted to a queue while it is in a pending state, and recorded into multiple primary command buffers

    // create a vulkan command buffer begin info struct, and use the provide vulkan macro to fill with default values
    VkCommandBufferBeginInfo begin_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin_info.flags = 0;                                                 // start at zero and depending on the bools in input will set certain flags -- think the ors make it so what ever the last flag selected is the one picked
    if (is_single_use) {                                                  // single use is set to tru
        begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;  // add the flag for a one time submit
    }
    if (is_renderpass_continue) {                                              // renderpass is set to true
        begin_info.flags |= VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;  // add the flag for render pass continue
    }
    if (is_simultaneous_use) {                                             // simultaneous use is set to true
        begin_info.flags |= VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;  // add the flag for simultaneous use
    }

    // use the vulkan fuction to begin the command buffer, pass in the handle to the command buffer, and the info created above and run against the vk check
    VK_CHECK(vkBeginCommandBuffer(command_buffer->handle, &begin_info));
    // if this passes then flip the command buffer state to recording
    command_buffer->state = COMMAND_BUFFER_STATE_RECORDING;
}

// vulkan command buffer end, just pass in a pointer to the command buffer being ended
void vulkan_command_buffer_end(vulkan_command_buffer* command_buffer) {
    VK_CHECK(vkEndCommandBuffer(command_buffer->handle));          // call the vulkan function to end a command buffer, passing it the handle to the command buffer being ended
    command_buffer->state = COMMAND_BUFFER_STATE_RECORDING_ENDED;  // if the check passed then set the command buffer state to recording ended
}

// call to update the command buffer to a submitted state, just needs a pointer to the command buffer
void vulkan_command_buffer_update_submitted(vulkan_command_buffer* command_buffer) {
    command_buffer->state = COMMAND_BUFFER_STATE_SUBMITTED;  // simply switches the command buffer state to submitted
}

// reset the vulkan command buffer, just needs a pointer to the command buffer to reset - resets it back to the ready state
void vulkan_command_buffer_reset(vulkan_command_buffer* command_buffer) {
    command_buffer->state = COMMAND_BUFFER_STATE_READY;  // simply switches the command buffer state to ready
}

// single use command buffer - pulls out uses and immediately puts back
// allocates and begins recording to out command buffer
void vulkan_command_buffer_allocate_and_begin_single_use(
    vulkan_context* context,                                                  // pass in the vulkan context
    VkCommandPool pool,                                                       // the pool that the command buffer belongs to
    vulkan_command_buffer* out_command_buffer) {                              // the command buffer being used
    vulkan_command_buffer_allocate(context, pool, TRUE, out_command_buffer);  // call our function to allocate a command buffer, pass through the context and the pool, set to primary, and the command buffer we are using comes out
    vulkan_command_buffer_begin(out_command_buffer, TRUE, FALSE, FALSE);      // call our function to begin a command buffer, give it the buffer, set single use to true, renderpass continue to false, and simulstaneous use to false
}

// ends recording, submits to and waits for queue operation and frees the provided command buffer
void vulkan_command_buffer_end_single_use(
    vulkan_context* context,                // pass in a pointer to the vukan context
    VkCommandPool pool,                     // the pool that the command buffer is going back too
    vulkan_command_buffer* command_buffer,  // a pointer to the command buffer being freed
    VkQueue queue) {                        // and the queue the command buffer had been used in

    // end the command buffer
    vulkan_command_buffer_end(command_buffer);

    // submit the queue
    VkSubmitInfo submit_info = {VK_STRUCTURE_TYPE_SUBMIT_INFO};  // create a vulkan submit info and use the provided macro to input default values into the fields
    submit_info.commandBufferCount = 1;                          // set the command buffer count to one
    submit_info.pCommandBuffers = &command_buffer->handle;       // give it the handle to the command buffer
    VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, 0));          // run the vulkan function to submit a queue, pass it the queue to submit, one command buffer, the info created above, and zero for the fence, since we arent ready to use them yet, against the vk check

    // wait for it to finish - using a vulkan function
    VK_CHECK(vkQueueWaitIdle(queue));

    // free the command buffer - using our function that takes in the context, the pool and the command buffer to be freed
    vulkan_command_buffer_free(context, pool, command_buffer);
}