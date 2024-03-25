// need to look these two things here up
#version 450
#extension GL_ARB_separate_shader_objects : enable

// allows us to take in individual vertex data and configure it to pass to the fragment shader
layout(location = 0) in vec3 in_position; // takes in a position for now

void main() {
    gl_Position = vec4(in_position, 1.0); // the final vertex position that gets passed on to the fragment shader - just going to pass in in_positon for now - has to take in a vec4, so vec3 gets converted. tack on 1.0 w coord
}