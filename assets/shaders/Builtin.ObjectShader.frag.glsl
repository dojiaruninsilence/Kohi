// the fragment shader determines the color that gets out put
// need to look these two things here up
#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 out_colour;

void main() {
    out_colour = vec4(1.0);
}