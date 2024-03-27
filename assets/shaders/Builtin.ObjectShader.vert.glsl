#version 450
#extension GL_ARB_separate_shader_objects : enable

// allows us to take in individual vertex data and configure it to pass to the fragment shader
// in from the application side
layout(location = 0) in vec3 in_position;

// very similar to a stucture in c
// each uniform will have a unique binding - its like the slot the uniform fits into
// set refers to a descriptor set
layout(set = 0, binding = 0) uniform global_uniform_object { // structure definition
    mat4 projection;
    mat4 view;
} global_ubo; // name variable

layout(push_constant) uniform push_constants {

    // only guaranteed a total of 128 bytes
    mat4 model; // 64 bytes
} u_push_constants;

void main() {
    // the final position sent to the fragment shader, is projection matrix times the view matrix times the position, when we add a model matrix, it will go in between view and position, the order is important
    gl_Position = global_ubo.projection * global_ubo.view * u_push_constants.model * vec4(in_position, 1.0); 
}