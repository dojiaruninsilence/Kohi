#include "vulkan_object_shader.h"

#include "core/logger.h"

#include "renderer/vulkan/vulkan_shader_utils.h"

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

    // descriptors

    return true;
}

// create a vulkan object shader, pass in a pointer to the context and a pointer to where the stucture for the shader is held
void vulkan_object_shader_destroy(vulkan_context* context, vulkan_object_shader* out_shader) {
}

// use a vulkan object shader, pass in a pointer to the context, and a pointer to where the shader struct is held
void vulkan_object_shader_use(vulkan_context* context, vulkan_object_shader* out_shader) {
}
