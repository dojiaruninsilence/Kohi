// the fragment shader determines the color that gets out put
// need to look these two things here up
#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 out_colour;

layout(set = 1, binding = 0) uniform local_uniform_object {
    vec4 diffuse_colour;
} object_ubo;

// samplers
layout(set = 1, binding = 1) uniform sampler2D diffuse_sampler;

// data transfer object
layout(location = 1) in struct dto {
    vec2 tex_coord;
} in_dto;

void main() {
    out_colour = object_ubo.diffuse_colour * texture(diffuse_sampler, in_dto.tex_coord);
}