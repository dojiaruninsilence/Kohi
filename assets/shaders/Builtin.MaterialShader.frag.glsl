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

struct point_light {
    vec3 position;
    vec4 colour;
    // usually 1, make sure denominator never gets smaller than 1
    float constant;
    // reduces light intensity linearly
    float linear;
    // makes the light fall off slower at longer distances
    float quadratic;
};

// TODO: feed in from the cpu
directional_light dir_light = {
    vec3(-0.57735, -0.57735, -0.57735),
    vec4(0.6, 0.6, 0.6, 1.0)
};

// TODO: feed in from the cpu
point_light p_light_0 = {
    vec3(-5.5, 0.0, -5.5),
    vec4(0.0, 1.0, 0.0, 1.0),
    1.0, // constant
    0.35, // linear
    0.44 // quadratic
};

// TODO: feed in from the cpu
point_light p_light_1 = {
    vec3(5.5, 0.0, -5.5),
    vec4(1.0, 0.0, 0.0, 1.0),
    1.0, // constant
    0.35, // linear
    0.44 // quadratic
};

// samplers, diffuse, spec
const int SAMP_DIFFUSE = 0;
const int SAMP_SPECULAR = 1;
const int SAMP_NORMAL = 2;
layout(set = 1, binding = 1) uniform sampler2D samplers[3];

layout(location = 0) flat in int in_mode; // never interpolated, always the same flat integer
// data transfer object
layout(location = 1) in struct dto {
    vec4 ambient;
    vec2 tex_coord;
    vec3 normal;
    vec3 view_position;
    vec3 frag_position;
    vec4 colour;
    vec4 tangent;
} in_dto;

mat3 TBN;

vec4 calculate_directional_light(directional_light light, vec3 normal, vec3 view_direction);
vec4 calculate_point_light(point_light light, vec3 normal, vec3 frag_position, vec3 view_direction);

void main() {
    vec3 normal = in_dto.normal;
    vec3 tangent = in_dto.tangent.xyz;
    tangent = (tangent - dot(tangent, normal) * normal); // gran smidtt process?
    vec3 bitangent = cross(in_dto.normal, in_dto.tangent.xyz) * in_dto.tangent.w; // cross product of tangent and normal multiplied by handedness, which will be 1 or -1 if the geometry is flipped
    TBN = mat3(tangent, bitangent, normal);

    // update the normal to use a sample from the normal map
    vec3 localNormal = 2.0 * texture(samplers[SAMP_NORMAL], in_dto.tex_coord).rgb - 1.0;
    normal = normalize(TBN * localNormal);

    if(in_mode == 0 || in_mode == 1) {
        vec3 view_direction = normalize(in_dto.view_position - in_dto.frag_position);

        out_colour = calculate_directional_light(dir_light, normal, view_direction);

        out_colour += calculate_point_light(p_light_0, normal, in_dto.frag_position, view_direction);
        out_colour += calculate_point_light(p_light_1, normal, in_dto.frag_position, view_direction);
    } else if(in_mode == 2) {
        out_colour = vec4(abs(normal), 1.0);
    }
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
    if(in_mode == 0) {
        diffuse *= diff_samp;
        ambient *= diff_samp;
        specular *= vec4(texture(samplers[SAMP_SPECULAR], in_dto.tex_coord).rgb, diffuse.a);
    }

    return (ambient + diffuse + specular);
}

vec4 calculate_point_light(point_light light, vec3 normal, vec3 frag_position, vec3 view_direction) {
    vec3 light_direction = normalize(light.position - frag_position);
    float diff = max(dot(normal, light_direction), 0.0);
    
    vec3 reflect_direction = reflect(-light_direction, normal);
    float spec = pow(max(dot(view_direction, reflect_direction), 0.0), object_ubo.shininess);

    // calculate attenuation, or light falloff over distance
    float distance = length(light.position - frag_position);
    float attenuation = 1.0 / (light.constant + light.linear * distance * light.quadratic * (distance * distance));

    vec4 ambient = in_dto.ambient;
    vec4 diffuse = light.colour * diff;
    vec4 specular = light.colour * spec;

    if(in_mode == 0) {
        vec4 diff_samp = texture(samplers[SAMP_DIFFUSE], in_dto.tex_coord);
        diffuse *= diff_samp;
        ambient *= diff_samp;
        specular *= vec4(texture(samplers[SAMP_SPECULAR], in_dto.tex_coord).rgb, diffuse.a);
    }

    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
    return (ambient + diffuse + specular);
}