#pragma once

#include "vulkan_types.inl"

// create a vulkan graphics pipeline
b8 vulkan_graphics_pipeline_create(
    vulkan_context* context,                        // pass in a pointer to the context
    vulkan_renderpass* renderpass,                  // pass in a pointer to a renderpass - one with the same setup
    u32 attribute_count,                            // pass in an attribute count
    VkVertexInputAttributeDescription* attributes,  // pass in the attributes
    u32 descriptor_set_layout_count,                // pass in the descriptor set layout count
    VkDescriptorSetLayout* descriptor_set_layouts,  // a pointer to the descriptor set layout
    u32 stage_count,                                // the number of stages
    VkPipelineShaderStageCreateInfo* stages,        // a pointer to a vulkan pipeline stage create info struct
    VkViewport viewport,                            // a vulkan viewport
    VkRect2D scissor,                               // scissor is the area that is rendered and what is clipped off
    b8 is_wireframe,                                // is it a wirframe
    vulkan_pipeline* out_pipeline);                 // pointer to where the pipeline created will be

// destroy a vulkan pipeline, just takes in the context and the pipeline to destroy
void vulkan_pipeline_destroy(vulkan_context* context, vulkan_pipeline* pipeline);

// bind a pipeline, pass in a pointer to a command buffer, the bind point, and a pointer to the pipeline to be bound
void vulkan_pipeline_bind(vulkan_command_buffer* command_buffer, VkPipelineBindPoint bind_point, vulkan_pipeline* pipeline);