#pragma once

#include "vulkan_types.inl"

// @brief creates a new vulkan pipeline
// @param context a pointer to the vulkan context
// @param renderpass a pointer to the renderpass to associate with the pipeline
// @param stride the stride of the vertex data to be used (ex: sizeof(vertex_3d))
// @param attribute_count the number of attributes
// @param attributes an array of attributes
// @param descriptor_set_layout_count the number of descriptor set layouts
// @param descriptor_set_layouts an array of descriptor set layouts
// @param stage_count the number of stages (vertex, fragment, etc)
// @param stages an array of stages
// @param viewport the viewport configuration
// @param scissor the scissor configuration
// @param cull_mode the face cull mode
// @param is_wireframe indicates if this pipeline should use wireframe mode
// @param depth_test_enabled indicates if depth testing is enabled for this pipeline
// @param out_pipeline a pointer to hold the newly-created pipeline
// @return true on success, otherwise false
b8 vulkan_graphics_pipeline_create(
    vulkan_context* context,
    vulkan_renderpass* renderpass,
    u32 stride,
    u32 attribute_count,
    VkVertexInputAttributeDescription* attributes,
    u32 descriptor_set_layout_count,
    VkDescriptorSetLayout* descriptor_set_layouts,
    u32 stage_count,
    VkPipelineShaderStageCreateInfo* stages,
    VkViewport viewport,
    VkRect2D scissor,
    face_cull_mode cull_mode,
    b8 is_wireframe,
    b8 depth_test_enabled,
    u32 push_constant_range_count,
    range* push_constant_ranges,
    vulkan_pipeline* out_pipeline);  // pointer to where the pipeline created will be

// destroy a vulkan pipeline, just takes in the context and the pipeline to destroy
void vulkan_pipeline_destroy(vulkan_context* context, vulkan_pipeline* pipeline);

// bind a pipeline, pass in a pointer to a command buffer, the bind point, and a pointer to the pipeline to be bound
void vulkan_pipeline_bind(vulkan_command_buffer* command_buffer, VkPipelineBindPoint bind_point, vulkan_pipeline* pipeline);