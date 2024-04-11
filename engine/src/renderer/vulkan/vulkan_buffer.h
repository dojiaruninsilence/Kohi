#pragma once

#include "vulkan_types.inl"

// create a vulkan buffer, pass in a pointer to the context, the size the buffer is going to need, what its usage is,
// and the type of memory needed, bool for bind on creation, and a pointer to where the resulting buffer will be held
b8 vulkan_buffer_create(
    vulkan_context* context,
    u64 size,
    VkBufferUsageFlagBits usage,
    u32 memory_property_flags,
    b8 bind_on_create,
    vulkan_buffer* out_buffer);

// destroy a vulkan buffer, just pass in the context and the buffer to destroy
void vulkan_buffer_destroy(vulkan_context* context, vulkan_buffer* buffer);

// resize the amount a buffer can hold - pass in a pointer to the context, the size that the context is being resized too,
// a pointer to the buffer being resized, a queue, and a command pool
b8 vulkan_buffer_resize(
    vulkan_context* context,
    u64 new_size,
    vulkan_buffer* buffer,
    VkQueue queue,
    VkCommandPool pool);

// bind a vulkan buffer - they cannot be used if they are not bound, pass in pointers to the context, the buffer being bound, and the offset of the buffer
void vulkan_buffer_bind(vulkan_context* context, vulkan_buffer* buffer, u64 offset);

// these 2 functions surround the loading of the data
void* vulkan_buffer_lock_memory(vulkan_context* context, vulkan_buffer* buffer, u64 offset, u64 size, u32 flags);
void vulkan_buffer_unlock_memory(vulkan_context* context, vulkan_buffer* buffer);

// @brief allocates space from a vulkan buffer. provides the offset at which the allocation occured.
// this will be required for data copying and freeing
// @param buffer buffer a pointer to the buffer form which to allocate
// @param size the size in bytes to be allocated
// @param out_offset a pointer to hold the offset in bytes from the beginning of the buffer.
// @return true on success; otherwise false
b8 vulkan_buffer_allocate(vulkan_buffer* buffer, u64 size, u64* out_offset);

// @brief frees space in the vulkan buffer
// @param buffer a pointer to the buffer to free data from
// @param size the size in bytes to be freed
// @param offset the offset in bytes from the beginning of the buffer
// @return true on success, other wise false
b8 vulkan_buffer_free(vulkan_buffer* buffer, u64 size, u64 offset);

// load data into a vulkan buffer -- pass in a pointers to the context and the buffer to
// load data into, pass in the offset, the size, and the flags. then the data to load
void vulkan_buffer_load_data(vulkan_context* context, vulkan_buffer* buffer, u64 offset, u64 size, u32 flags, const void* data);

// copy data from one buffer to another, pass in a pointer to the context, a command pool, a fence, a queue,
// the buffer being copied, how big the source buffer is, and the offset of the source buffer, the destination buffer, and its offset
void vulkan_buffer_copy_to(
    vulkan_context* context,
    VkCommandPool pool,
    VkFence fence,
    VkQueue queue,
    VkBuffer source,
    u64 source_offset,
    VkBuffer dest,
    u64 dest_offset,
    u64 size);