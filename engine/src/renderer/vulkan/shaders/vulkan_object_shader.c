#include "vulkan_object_shader.h"

#include "core/logger.h"
#include "core/kmemory.h"
#include "math/math_types.h"

#include "renderer/vulkan/vulkan_shader_utils.h"
#include "renderer/vulkan/vulkan_pipeline.h"

#define BUILTIN_SHADER_NAME_OBJECT "Builtin.ObjectShader"  // this is part of the filename that we are going to load will be concatenated with the stage name and extension -- for custom shaders and such this will be different

// create a vulkan object shader, pass in a pointer to the context and a pointer to where the stucture for the shader will be held
b8 vulkan_object_shader_create(vulkan_context* context, vulkan_object_shader* out_shader) {
    // shader module initialize per stage
    char stage_type_strs[OBJECT_SHADER_STAGE_COUNT][5] = {"vert", "frag"};  // set up a short array of strings to obtain the various stage names that we will need for this shader
    // store the stage types - use the shader stage count for the size, and input vulkan macros for the types(vertex and fragment for now)
    VkShaderStageFlagBits stage_types[OBJECT_SHADER_STAGE_COUNT] = {VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT};

    for (u32 i = 0; i < OBJECT_SHADER_STAGE_COUNT; ++i) {  // iterate through all the stages
        // create shader modules - contains the logic for programmable shaders
        // use our funtion, pass in the context, the shader name object, the stage name at index i, the stage type at index i, and a pointer to the resulting shader stages array
        if (!create_shader_module(context, BUILTIN_SHADER_NAME_OBJECT, stage_type_strs[i], stage_types[i], i, out_shader->stages)) {  // if it fails
            KERROR("Unable to create %s shader module for '%s'.", stage_type_strs[i], BUILTIN_SHADER_NAME_OBJECT);                    // throw an error
            return false;                                                                                                             // boot out
        }
    }

    // TODO: descriptors

    // pipeline creation
    // define the viewport to pass into the pipeline
    VkViewport viewport;                                  // define a vulkan viewport
    viewport.x = 0.0f;                                    // x is set to 0
    viewport.y = (f32)context->framebuffer_height;        // y to the height of the framebuffer, because we have to flip the y axis - to match opengl
    viewport.width = (f32)context->framebuffer_width;     // pass in the framebuffer width
    viewport.height = -(f32)context->framebuffer_height;  // and height is negative, to flip the y axis - to match opengl
    viewport.minDepth = 0.0f;                             // min depth is 0, think these are 2d settings
    viewport.maxDepth = 1.0f;                             // max depth is 1

    // define the scissor - define what is displayed and what is clipped
    VkRect2D scissor;
    scissor.offset.x = scissor.offset.y = 0;              // set both x and y  offsets to 0, believe this means it lines up with the viewport
    scissor.extent.width = context->framebuffer_width;    // pass in the width
    scissor.extent.height = context->framebuffer_height;  // and height from the framebuffer

    // define the attributes - making this expandable
    u32 offset = 0;                                                             // as we add attributes, this will help us offset new ones in bytes
    const i32 attribute_count = 1;                                              // starting with only one attribute
    VkVertexInputAttributeDescription attribute_descriptions[attribute_count];  // create an array of vulkan attributes
    // position
    VkFormat formats[attribute_count] = {
        // create an array of vulkan formats, these are for the stuff passed to the glsl files
        VK_FORMAT_R32G32B32_SFLOAT  // format for our only attribute at the moment - believe this is for a vec3 with 32 bit floating point elements
    };
    u64 sizes[attribute_count] = {
        // create an array that will have size values, that correspond to the attributes at same index in format array
        sizeof(vec3)  // only attribute for now is a vec3(position)
    };
    for (u32 i = 0; i < attribute_count; ++i) {         // iterate through the attributes
        attribute_descriptions[i].binding = 0;          // set binding index to zero
        attribute_descriptions[i].location = i;         // location to i - this location is the same location in glsl files, this is where it knows to send it
        attribute_descriptions[i].format = formats[i];  // set to corresponding format
        attribute_descriptions[i].offset = offset;      // pass in the offset
        offset += sizes[i];                             // increment the offset
    }

    // TODO: descriptor set layouts

    // stages
    // NOTE: should match the number of shader->stages
    VkPipelineShaderStageCreateInfo stage_create_infos[OBJECT_SHADER_STAGE_COUNT];           // define an array vulkan shader stage create infos with stage count macro as the size
    kzero_memory(stage_create_infos, sizeof(stage_create_infos));                            // zero out the memory for the array
    for (u32 i = 0; i < OBJECT_SHADER_STAGE_COUNT; ++i) {                                    // iterate through the stage count
        stage_create_infos[i].sType = out_shader->stages[i].shader_stage_create_info.sType;  // copy over the stype
        stage_create_infos[i] = out_shader->stages[i].shader_stage_create_info;              // and the shader create infos
    }

    // create the graphics pipeline
    if (!vulkan_graphics_pipeline_create(  // and check to see if it fails
            context,                       // pass it the context
            &context->main_renderpass,     // the main renderpass
            attribute_count,               // attribute count
            attribute_descriptions,        // attribute array
            0,                             // no descriptors
            0,                             // no descriptors
            OBJECT_SHADER_STAGE_COUNT,     // give it the stage count
            stage_create_infos,            // the needed create infos
            viewport,                      // the viewport defined for the pipeline
            scissor,                       // the scissor
            false,                         // is not wire frame
            &out_shader->pipeline)) {      // and the address to where the pipline will be held
        KERROR("Failed to load graphics pipeline for object shader.");
        return false;
    }

    return true;
}

// create a vulkan object shader, pass in a pointer to the context and a pointer to where the stucture for the shader is held
void vulkan_object_shader_destroy(vulkan_context* context, vulkan_object_shader* shader) {
    // destroy the pipeline
    vulkan_pipeline_destroy(context, &shader->pipeline);

    // destroy shader modules
    for (u32 i = 0; i < OBJECT_SHADER_STAGE_COUNT; ++i) {                                                     // iterate through all of the stages
        vkDestroyShaderModule(context->device.logical_device, shader->stages[i].handle, context->allocator);  // use the vulkan function to destoy each module
        shader->stages[i].handle = 0;                                                                         // and reset each of the handles
    }
}

// use a vulkan object shader, pass in a pointer to the context, and a pointer to where the shader struct is held
void vulkan_object_shader_use(vulkan_context* context, vulkan_object_shader* shader) {
    u32 image_index = context->image_index;  // get the current image index from the context
    // bind the pipline, pass in the graphics command buffer at the current image index, bind to graphics bind point of the pipeline, and an address to the pipline to bind
    vulkan_pipeline_bind(&context->graphics_command_buffers[image_index], VK_PIPELINE_BIND_POINT_GRAPHICS, &shader->pipeline);
}
