#pragma once

#include "renderer/vulkan/vulkan_types.inl"
#include "renderer/renderer_types.inl"

// create a vulkan object shader, pass in a pointer to the context and a pointer to where the stucture for the shader will be held
b8 vulkan_object_shader_create(vulkan_context* context, vulkan_object_shader* out_shader);

// create a vulkan object shader, pass in a pointer to the context and a pointer to where the stucture for the shader is held
void vulkan_object_shader_destroy(vulkan_context* context, vulkan_object_shader* out_shader);

// use a vulkan object shader, pass in a pointer to the context, and a pointer to where the shader struct is held
void vulkan_object_shader_use(vulkan_context* context, vulkan_object_shader* out_shader);
