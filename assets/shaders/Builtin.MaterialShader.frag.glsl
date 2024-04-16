// the fragment shader determines the color that gets out put
// need to look these two things here up
#version 450

layout(location = 0) out vec4 out_colour;

layout(set = 1, binding = 0) uniform local_uniform_object {
    vec4 diffuse_colour;
} object_ubo;

struct directional_light {
    vec3 direction;
    vec4 colour;
};

// TODO: feed in from the cpu
directional_light dir_light = {
    vec3(-0.57735, -0.57735, 0.57735),
    vec4(0.8, 0.8, 0.8, 1.0)
};

// samplers
layout(set = 1, binding = 1) uniform sampler2D diffuse_sampler;

// data transfer object
layout(location = 1) in struct dto {
    vec4 ambient;
    vec2 tex_coord;
    vec3 normal;
} in_dto;

vec4 calculate_directional_light(directional_light light, vec3 normal);

void main() {
    out_colour = calculate_directional_light(dir_light, in_dto.normal);
}

vec4 calculate_directional_light(directional_light light, vec3 normal) {
    float diffuse_factor = max(dot(normal, -light.direction), 0.0); // compare the direction of the light to the direction of the normal, to determine how lit it is. dot is dot product look up

    vec4 diff_samp = texture(diffuse_sampler, in_dto.tex_coord);
    vec4 ambient = vec4(vec3(in_dto.ambient * object_ubo.diffuse_colour), diff_samp.a); // unlit stuff uses the ambient light color
    vec4 diffuse = vec4(vec3(light.colour * diffuse_factor), diff_samp.a); // use the diffuse factor for the actually lit stuff

    // combine the diffuse and ambient into the diffuse sample
    diffuse *= diff_samp;
    ambient *= diff_samp;

    return (ambient + diffuse);
}