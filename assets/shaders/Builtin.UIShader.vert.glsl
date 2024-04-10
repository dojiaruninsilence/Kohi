#version 450

// allows us to take in individual vertex data and configure it to pass to the fragment shader
// in from the application side
layout(location = 0) in vec2 in_position;
layout(location = 1) in vec2 in_texcoord;

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

// not using yet, but needs to be in place
layout(location = 0) out int out_mode;

// data transfer object
layout(location = 1) out struct dto {
    vec2 tex_coord; // pass the tex coords to the frag shader
} out_dto;

void main() {
    // NOTE: intentionally flip y texture coordinate. this, along with flipped ortho matrix, puts [0, 0] in the top left
    // instead of the bottom left and adjusts texture coordinates to show in the right direction
    out_dto.tex_coord = vec2(in_texcoord.x, 1.0 - in_texcoord.y); // pass the tex coords to the frag shader
    // the final position sent to the fragment shader, is projection matrix times the view matrix times the position, when we add a model matrix, it will go in between view and position, the order is important
    gl_Position = global_ubo.projection * global_ubo.view * u_push_constants.model * vec4(in_position, 0.0, 1.0);
}