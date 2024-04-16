// the fragment shader determines the color that gets out put
// need to look these two things here up
#version 450

layout(location = 0) out vec4 out_colour;

layout(set = 1, binding = 0) uniform local_uniform_object {
    vec4 diffuse_colour;
    float shininess;
} object_ubo;

struct directional_light {
    vec3 direction;
    vec4 colour;
};

// TODO: feed in from the cpu
directional_light dir_light = {
    vec3(-0.57735, -0.57735, -0.57735),
    vec4(0.8, 0.8, 0.8, 1.0)
};

// samplers, diffuse, spec
const int SAMP_DIFFUSE = 0;
const int SAMP_SPECULAR = 1;
layout(set = 1, binding = 1) uniform sampler2D samplers[2];

// data transfer object
layout(location = 1) in struct dto {
    vec4 ambient;
    vec2 tex_coord;
    vec3 normal;
    vec3 view_position;
    vec3 frag_position;
} in_dto;

vec4 calculate_directional_light(directional_light light, vec3 normal, vec3 view_direction);

void main() {
    vec3 view_direction = normalize(in_dto.view_position - in_dto.frag_position);

    out_colour = calculate_directional_light(dir_light, in_dto.normal, view_direction);
}

vec4 calculate_directional_light(directional_light light, vec3 normal, vec3 view_direction) {
    float diffuse_factor = max(dot(normal, -light.direction), 0.0); // compare the direction of the light to the direction of the normal, to determine how lit it is. dot is dot product look up

    vec3 half_direction = normalize(view_direction - light.direction);
    // dot product is the difference betweent the 2, max makes sure there arent any negative values, and pow sets to the power of the shininess
    float specular_factor = pow(max(dot(half_direction, normal), 0.0), object_ubo.shininess);

    vec4 diff_samp = texture(samplers[SAMP_DIFFUSE], in_dto.tex_coord);
    vec4 ambient = vec4(vec3(in_dto.ambient * object_ubo.diffuse_colour), diff_samp.a); // unlit stuff uses the ambient light color
    vec4 diffuse = vec4(vec3(light.colour * diffuse_factor), diff_samp.a); // use the diffuse factor for the actually lit stuff
    vec4 specular = vec4(vec3(light.colour * specular_factor), diff_samp.a);

    // combine the diffuse and ambient into the diffuse sample
    diffuse *= diff_samp;
    ambient *= diff_samp;
    specular *= vec4(texture(samplers[SAMP_SPECULAR], in_dto.tex_coord).rgb, diffuse.a);

    return (ambient + diffuse + specular);
}