#include "vulkan_pipeline.h"
#include "vulkan_utils.h"

#include "core/kmemory.h"
#include "core/logger.h"

#include "math/math_types.h"

// create a vulkan graphics pipeline
b8 vulkan_graphics_pipeline_create(                 // returns a bool
    vulkan_context* context,                        // pass in a pointer to the context
    vulkan_renderpass* renderpass,                  // pass in a pointer to a renderpass - one with the same setup
    u32 stride,                                     // the amnt of memory each element(vertex 3d) will take for offsetting purposes
    u32 attribute_count,                            // pass in an attribute count
    VkVertexInputAttributeDescription* attributes,  // pass in the attributes
    u32 descriptor_set_layout_count,                // pass in the descriptor set layout count
    VkDescriptorSetLayout* descriptor_set_layouts,  // a pointer to the descriptor set layout
    u32 stage_count,                                // the number of stages
    VkPipelineShaderStageCreateInfo* stages,        // a pointer to a vulkan pipeline stage create info struct
    VkViewport viewport,                            // a vulkan viewport
    VkRect2D scissor,                               // scissor is the area that is rendered and what is clipped off
    b8 is_wireframe,                                // is it a wirframe
    b8 depth_test_enabled,                          // for depth attachments and such
    vulkan_pipeline* out_pipeline) {                // pointer to where the pipeline created will be
    // view port state
    // create the vulkan struct for creating a pipeline viewport state, use the macro to format and fill with default values
    VkPipelineViewportStateCreateInfo viewport_state = {VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    viewport_state.viewportCount = 1;       // only one viewport for this pipeline
    viewport_state.pViewports = &viewport;  // the address to the viewport
    viewport_state.scissorCount = 1;        // only one viewport, so only one scissor
    viewport_state.pScissors = &scissor;    // pass in the address to the scissor

    // rasterizer
    // create the vulkan struct for creating a pipeline rasterization state, use the macro to format and fill with default values
    VkPipelineRasterizationStateCreateInfo rasterizer_create_info = {VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    rasterizer_create_info.depthClampEnable = VK_FALSE;                                               // set the depth clamping to off
    rasterizer_create_info.rasterizerDiscardEnable = VK_FALSE;                                        // rasterizer discard enabled is turned off
    rasterizer_create_info.polygonMode = is_wireframe ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;  // set polygon mode to line if wireframe is enabled, fill if not
    rasterizer_create_info.lineWidth = 1.0f;                                                          // belive this would be for the wire frame mode
    rasterizer_create_info.cullMode = VK_CULL_MODE_BACK_BIT;                                          // set the cull mode to back bit - cull the back faces, so they dont show
    rasterizer_create_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;                               // believe this means that the z axis positive points out of the screen
    rasterizer_create_info.depthBiasEnable = VK_FALSE;                                                // depth bias is disabled
    rasterizer_create_info.depthBiasConstantFactor = 0.0f;
    rasterizer_create_info.depthBiasClamp = 0.0f;
    rasterizer_create_info.depthBiasSlopeFactor = 0.0f;

    // multisampling - not using right away, so most are set to off, and set to one bit sampling
    // create the vulkan struct for creating a pipeline multisample state, use the macro to format and fill with default values
    VkPipelineMultisampleStateCreateInfo multisampling_create_info = {VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    multisampling_create_info.sampleShadingEnable = VK_FALSE;                // sample shading disabled
    multisampling_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;  // set rasterization samples to 1 bit
    multisampling_create_info.minSampleShading = 1.0f;                       // min sample shading set to 1.0f
    multisampling_create_info.pSampleMask = 0;
    multisampling_create_info.alphaToCoverageEnable = VK_FALSE;
    multisampling_create_info.alphaToOneEnable = VK_FALSE;

    // depth and stencil testing
    // create the vulkan struct for creating a pipeline depth stencil state, use the macro to format and fill with default values
    VkPipelineDepthStencilStateCreateInfo depth_stencil = {VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    if (depth_test_enabled) {
        depth_stencil.depthTestEnable = VK_TRUE;            // depth testing enabled, believe this is 3d stuffs
        depth_stencil.depthWriteEnable = VK_TRUE;           // depth write enabled - enables writing to the depth buffer
        depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS;  // depth compare operation is set to less - use less than in comparisons for depth - for what gets clipped
        depth_stencil.depthBoundsTestEnable = VK_FALSE;     // not using now
        depth_stencil.stencilTestEnable = VK_FALSE;
    }

    // color blending -- defines how colors are written to the image
    VkPipelineColorBlendAttachmentState color_blend_attachement_state;                          // create a vulkan struct
    kzero_memory(&color_blend_attachement_state, sizeof(VkPipelineColorBlendAttachmentState));  // zero out the memory for the vulkan struct
    color_blend_attachement_state.blendEnable = VK_TRUE;                                        // color blend is enabled, for tranparencies?
    color_blend_attachement_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;              //  blend factor uses the alpha channel
    color_blend_attachement_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_blend_attachement_state.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachement_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    color_blend_attachement_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_blend_attachement_state.alphaBlendOp = VK_BLEND_OP_ADD;

    // what channels we want the pipeline to write too
    color_blend_attachement_state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                                   VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    // create the vulkan struct for creating a pipeline color blend state, use the macro to format and fill with default values
    VkPipelineColorBlendStateCreateInfo color_blend_state_create_info = {VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    color_blend_state_create_info.logicOpEnable = VK_FALSE;
    color_blend_state_create_info.logicOp = VK_LOGIC_OP_COPY;
    color_blend_state_create_info.attachmentCount = 1;                            // only the struct
    color_blend_state_create_info.pAttachments = &color_blend_attachement_state;  // pass in the stuct created prvious to this one

    // dynamic state -- pipelines are not changeble, but a few things can change a bit - these can be changed on the fly
    const u32 dynamic_state_count = 3;
    VkDynamicState dynamic_states[dynamic_state_count] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
        VK_DYNAMIC_STATE_LINE_WIDTH};

    // create the vulkan struct for creating a pipeline dynamic state, use the macro to format and fill with default values
    VkPipelineDynamicStateCreateInfo dynamic_state_create_info = {VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    dynamic_state_create_info.dynamicStateCount = dynamic_state_count;  // give the count of the number of states that are dynamic
    dynamic_state_create_info.pDynamicStates = dynamic_states;          // pass the array of states

    // vertex input
    VkVertexInputBindingDescription binding_description;          // create vulkan binding description struct
    binding_description.binding = 0;                              // the binding index
    binding_description.stride = stride;                          //  stride is the amount of bytes in an element
    binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;  // move to next data entry for each vertex

    // attributes
    // create the vulkan struct for creating a pipeline vertex input state, use the macro to format and fill with default values
    VkPipelineVertexInputStateCreateInfo vertex_input_info = {VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vertex_input_info.vertexBindingDescriptionCount = 1;                  // one set of descriptions, created prior to this
    vertex_input_info.pVertexBindingDescriptions = &binding_description;  // pass in the description created above
    vertex_input_info.vertexAttributeDescriptionCount = attribute_count;  // attributes are the layout things we pass into the glsl files , this is the count
    vertex_input_info.pVertexAttributeDescriptions = attributes;          // and these are the actual attributes - this is where the vertex 3d is split up to be fed into the glsl fthing, shader?

    // Input assembly - takes all of the info from above and assembles it in a way that the vertex shader can use
    // create the vulkan struct for creating a pipeline input assembly, use the macro to format and fill with default values
    VkPipelineInputAssemblyStateCreateInfo input_assembly = {VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;  // means that all of our geometry is expected to be triangle based
    input_assembly.primitiveRestartEnable = VK_FALSE;

    // pipeline layout
    // create the vulkan struct for creating a pipeline layout, use the macro to format and fill with default values
    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};

    // push constants
    VkPushConstantRange push_constant;
    push_constant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;             // for the vertex shader stage
    push_constant.offset = sizeof(mat4) * 0;                           // 0 for now
    push_constant.size = sizeof(mat4) * 2;                             // equals 128 bytes, which is the max we want to use for push constants
    pipeline_layout_create_info.pushConstantRangeCount = 1;            // only one push constant for now
    pipeline_layout_create_info.pPushConstantRanges = &push_constant;  // array of push constants

    // descriptor set layouts - this has to do with uniforms
    pipeline_layout_create_info.setLayoutCount = descriptor_set_layout_count;
    pipeline_layout_create_info.pSetLayouts = descriptor_set_layouts;

    // create the pipeline layout
    VK_CHECK(vkCreatePipelineLayout(       // check against vk check
        context->device.logical_device,    // pass in the logical device
        &pipeline_layout_create_info,      // the layout create info created above
        context->allocator,                // the memory allocation stuffs
        &out_pipeline->pipeline_layout));  // the address to where the layout is going to be held

    // pipeline create
    VkGraphicsPipelineCreateInfo pipeline_create_info = {VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    pipeline_create_info.stageCount = stage_count;                // pass in the stage count
    pipeline_create_info.pStages = stages;                        // pass in the array of stages
    pipeline_create_info.pVertexInputState = &vertex_input_info;  // pass in the vertex input info created above
    pipeline_create_info.pInputAssemblyState = &input_assembly;   // and the input assembly info created above

    // pass in all the create info structs created above
    pipeline_create_info.pViewportState = &viewport_state;  // pass in the viewport state info created above
    pipeline_create_info.pRasterizationState = &rasterizer_create_info;
    pipeline_create_info.pMultisampleState = &multisampling_create_info;
    pipeline_create_info.pDepthStencilState = depth_test_enabled ? &depth_stencil : 0;
    pipeline_create_info.pColorBlendState = &color_blend_state_create_info;
    pipeline_create_info.pDynamicState = &dynamic_state_create_info;
    pipeline_create_info.pTessellationState = 0;  // nothing here for now

    pipeline_create_info.layout = out_pipeline->pipeline_layout;  // created above

    pipeline_create_info.renderPass = renderpass->handle;  // pass in the handle to the renderpass
    pipeline_create_info.subpass = 0;
    pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_create_info.basePipelineIndex = -1;

    // actually create the pipeline
    VkResult result = vkCreateGraphicsPipelines(
        context->device.logical_device,  // pass in the logigal device
        VK_NULL_HANDLE,                  // no pipeline caching
        1,                               // only creating one
        &pipeline_create_info,           // pass in all the info created above
        context->allocator,              // pass in memory allocation stuffs
        &out_pipeline->handle);          // pass in a handle to the pipeline to be created

    if (vulkan_result_is_success(result)) {  // check the results
        KDEBUG("Graphics pipeline created!");
        return true;  // boot out success
    }

    // if the result is not a success
    KERROR("vkCreateGraphicsPipelines failed with %s.", vulkan_result_string(result, true));
    return false;  // boot out fail
}

// destroy a vulkan pipeline, just takes in the context and the pipeline to destroy
void vulkan_pipeline_destroy(vulkan_context* context, vulkan_pipeline* pipeline) {
    if (pipeline) {  // if there is actually a pipeline to destroy
        // destroy the pipeline
        if (pipeline->handle) {                                                                       // make sure there is a handle to the pipeline}
            vkDestroyPipeline(context->device.logical_device, pipeline->handle, context->allocator);  // use vulkan func to destroy the pipeline
            pipeline->handle = 0;                                                                     // reset the handle
        }

        // destroy the layout
        if (pipeline->pipeline_layout) {                                                                             // if there is a layout
            vkDestroyPipelineLayout(context->device.logical_device, pipeline->pipeline_layout, context->allocator);  // use the vulkan func to destroy the layout
            pipeline->pipeline_layout = 0;                                                                           // reset the layout
        }
    }
}

// bind a pipeline, pass in a pointer to a command buffer, the bind point, and a pointer to the pipeline to be bound
void vulkan_pipeline_bind(vulkan_command_buffer* command_buffer, VkPipelineBindPoint bind_point, vulkan_pipeline* pipeline) {
    vkCmdBindPipeline(command_buffer->handle, bind_point, pipeline->handle);  // call vulkan function to bind the pipeline, pass in a command buffer handle to command the bind, the point to bind it, and the pipeline to bind
}