#pragma once

#include "vulkan_types.inl"

// create a vulkan shader module
b8 create_shader_module(
    vulkan_context* context,                  // pass in a pointer to the context
    const char* name,                         // pass in the shader name object, this will concantenate with the
    const char* type_str,                     // type string(vert, frag, ect), and the extension to get the filename
    VkShaderStageFlagBits shader_stage_flag,  // also pass in the vulkan stage type(vertex, fragment, ect)
    u32 stage_index,                          // the stage index(i)
    vulkan_shader_stage* shader_stages);      // and the array of vulkan shader stages