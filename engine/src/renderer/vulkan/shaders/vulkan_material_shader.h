#pragma once

#include "renderer/vulkan/vulkan_types.inl"
#include "renderer/renderer_types.inl"

// create a vulkan object shader, pass in a pointer to the context and a pointer to where the stucture for the shader will be held
b8 vulkan_material_shader_create(vulkan_context* context, vulkan_material_shader* out_shader);

// create a vulkan object shader, pass in a pointer to the context and a pointer to where the stucture for the shader is held
void vulkan_material_shader_destroy(vulkan_context* context, struct vulkan_material_shader* shader);

// use a vulkan object shader, pass in a pointer to the context, and a pointer to where the shader struct is held
void vulkan_material_shader_use(vulkan_context* context, struct vulkan_material_shader* shader);

// update the object shaders global state, , pass in a pointer to the context, and a pointer to where the shader struct is held
void vulkan_material_shader_update_global_state(vulkan_context* context, struct vulkan_material_shader* shader, f32 delta_time);

// this is only temporary, this will be moving
void vulkan_material_shader_update_object(vulkan_context* context, struct vulkan_material_shader* shader, geometry_render_data data);

// pull in resources from outside the engine like meshes, and textures and such, and tag it with an id, to keep track of them without having to keep pointers
b8 vulkan_material_shader_acquire_resources(vulkan_context* context, struct vulkan_material_shader* shader, material* material);
void vulkan_material_shader_release_resources(vulkan_context* context, struct vulkan_material_shader* shader, material* material);