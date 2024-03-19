#pragma once
#include "vulkan_types.inl"

// vulkan command buffer allocation -- we dont create them, we allocate them from a command pool -- taking it out of the command pool
void vulkan_command_buffer_allocate(
    vulkan_context* context,                     // take in the vulkan context
    VkCommandPool pool,                          // the pool that will be allocated from
    b8 is_primary,                               // primary if set to true. secondary buffers can ne used inside of another command buffer
    vulkan_command_buffer* out_command_buffer);  // what writes out to the command buffer

// vulkan command buffer free -- since its an allocation we free rather than destroy - this would be the putting it back into the command pool
void vulkan_command_buffer_free(
    vulkan_context* context,                 // take in a pointer to the vulkan context
    VkCommandPool pool,                      // the pool that the command buffer came from and will return too
    vulkan_command_buffer* command_buffer);  // the pointer to the command buffer being freed

// vulkan command buffer begin -- this is the begin render recording state
void vulkan_command_buffer_begin(
    vulkan_command_buffer* command_buffer,  // pass a pointer to the command buffer to begin
    b8 is_single_use,
    b8 is_renderpass_continue,
    b8 is_simultaneous_use);

// vulkan command buffer end, just pass in a pointer to the command buffer being ended
void vulkan_command_buffer_end(vulkan_command_buffer* command_buffer);

// call to update the command buffer to a submitted state, just needs a pointer to the command buffer
void vulkan_command_buffer_update_submitted(vulkan_command_buffer* command_buffer);

// reset the vulkan command buffer, just needs a pointer to the command buffer to reset - resets it back to the ready state
void vulkan_command_buffer_reset(vulkan_command_buffer* command_buffer);

// single use command buffer - pulls out uses and immediately puts back
// allocates and begins recording to out command buffer
void vulkan_command_buffer_allocate_and_begin_single_use(
    vulkan_context* context,                     // pass in the vulkan context
    VkCommandPool pool,                          // the pool that the command buffer belongs to
    vulkan_command_buffer* out_command_buffer);  // the command buffer being used

// ends recording, submits to and waits for queue operation and frees the provided command buffer
void vulkan_command_buffer_end_single_use(
    vulkan_context* context,                // pass in a pointer to the vukan context
    VkCommandPool pool,                     // the pool that the command buffer is going back too
    vulkan_command_buffer* command_buffer,  // a pointer to the command buffer being freed
    VkQueue queue);                         // and the queue the command buffer had been used in