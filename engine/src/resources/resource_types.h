#pragma once

#include "math/math_types.h"

// predefined resource types
typedef enum resource_type {
    RESOURCE_TYPE_TEXT,
    RESOURCE_TYPE_BINARY,
    RESOURCE_TYPE_IMAGE,
    RESOURCE_TYPE_MATERIAL,
    RESOURCE_TYPE_STATIC_MESH,
    // @brief shader resource type(or more accurately shader config)
    RESOURCE_TYPE_SHADER,
    // @brief mesh resource type (collection of geometry configs)
    RESOURCE_TYPE_MESH,
    RESOURCE_TYPE_CUSTOM
} resource_type;

// struct that represents the resource itself
typedef struct resource {
    u32 loader_id;     // id of the loader to use for the resource
    const char* name;  // the name of the resource
    char* full_path;   // file path of the resource
    u64 data_size;     // size of the data in the resource
    void* data;        // pointer to the actual data
} resource;

// different structs for different data types

// image resource data structure - how data for images is stored
typedef struct image_resource_data {
    u8 channel_count;  // number of channeles in the image
    u32 width;         // image width
    u32 height;        // image height
    u8* pixels;        // pointer to the pixel data
} image_resource_data;

// define how long a texture name is allowed to be
#define TEXTURE_NAME_MAX_LENGTH 512

// where we will store data for textures - interface
typedef struct texture {
    u32 id;               // id of the texture
    u32 width;            // width of the texture
    u32 height;           // height of the texture
    u8 channel_count;     // how many channels it has (rgba channels)
    b8 has_transparency;  // bool to store whether it has transparencies that need to be considered. alpha channel or not
    u32 generation;
    char name[TEXTURE_NAME_MAX_LENGTH];
    void* internal_data;  // graphics api specific data
} texture;

// a list of different uses textures may have
typedef enum texture_use {
    TEXTURE_USE_UNKNOWN = 0x00,
    TEXTURE_USE_MAP_DIFFUSE = 0x01,  // textures with this tag will be used as a diffuse map
    // @brief the texture is used as a specular map
    TEXTURE_USE_MAP_SPECULAR = 0x02,
    // @brief the texture is used as a normal map
    TEXTURE_USE_MAP_NORMAL = 0x03
} texture_use;

// store data for texture maps
typedef struct texture_map {
    texture* texture;
    texture_use use;
} texture_map;

// define the max length that a material may be named
#define MATERIAL_NAME_MAX_LENGTH 256

// where we will hold the configuration settings for each of the materials
typedef struct material_config {
    char name[MATERIAL_NAME_MAX_LENGTH];
    char* shader_name;  // type like ui or world
    b8 auto_release;    // does it auto release
    vec4 diffuse_colour;
    // @brief the shininess of the material
    f32 shininess;
    char diffuse_map_name[TEXTURE_NAME_MAX_LENGTH];  // ultimately becomes the texture name, for loading and unloading purposes
    // @brief the specular map name
    char specular_map_name[TEXTURE_NAME_MAX_LENGTH];
    // @brief the normal map name
    char normal_map_name[TEXTURE_NAME_MAX_LENGTH];
} material_config;

// where we will store the data for the materials
typedef struct material {
    u32 id;                               // the id of the material
    u32 generation;                       // update whenever the material changes
    u32 internal_id;                      // id handle to the internal material data
    char name[MATERIAL_NAME_MAX_LENGTH];  // name of the material
    vec4 diffuse_colour;
    texture_map diffuse_map;  // store array of textures and their uses

    // @brief the specular texture map
    texture_map specular_map;
    // @brief the normal texture map
    texture_map normal_map;

    // @brief the material shininess, determines how concentrated the specular lighting is
    f32 shininess;

    u32 shader_id;

    // @brief synced to the renderer's current frame number when the material has been applied that frame
    u32 render_frame_number;
} material;

#define GEOMETRY_NAME_MAX_LENGTH 256

// @brief represents actual geometry in the world
// typically (but not always, depending on use) paired with a material
typedef struct geometry {
    u32 id;           // unique id
    u32 internal_id;  // renderer specific id
    u32 generation;   // track any time it is updated
    char name[GEOMETRY_NAME_MAX_LENGTH];
    material* material;
} geometry;

typedef struct mesh {
    u16 geometry_count;
    geometry** geometries;  // an array of pointers, not the acutal geometries
    transform transform;
} mesh;

// @brief shader stages available in the system
typedef enum shader_stage {
    SHADER_STAGE_VERTEX = 0x00000001,
    SHADER_STAGE_GEOMETRY = 0x00000002,
    SHADER_STAGE_FRAGMENT = 0x00000004,
    SHADER_STAGE_COMPUTE = 0x0000008
} shader_stage;

// @brief available attribute types
typedef enum shader_attribute_type {
    SHADER_ATTRIB_TYPE_FLOAT32 = 0U,
    SHADER_ATTRIB_TYPE_FLOAT32_2 = 1U,
    SHADER_ATTRIB_TYPE_FLOAT32_3 = 2U,
    SHADER_ATTRIB_TYPE_FLOAT32_4 = 3U,
    SHADER_ATTRIB_TYPE_MATRIX_4 = 4U,
    SHADER_ATTRIB_TYPE_INT8 = 5U,
    SHADER_ATTRIB_TYPE_UINT8 = 6U,
    SHADER_ATTRIB_TYPE_INT16 = 7U,
    SHADER_ATTRIB_TYPE_UINT16 = 8U,
    SHADER_ATTRIB_TYPE_INT32 = 9U,
    SHADER_ATTRIB_TYPE_UINT32 = 10U,
} shader_attribute_type;

// @brief available uniform types
typedef enum shader_uniform_type {
    SHADER_UNIFORM_TYPE_FLOAT32 = 0U,
    SHADER_UNIFORM_TYPE_FLOAT32_2 = 1U,
    SHADER_UNIFORM_TYPE_FLOAT32_3 = 2U,
    SHADER_UNIFORM_TYPE_FLOAT32_4 = 3U,
    SHADER_UNIFORM_TYPE_INT8 = 4U,
    SHADER_UNIFORM_TYPE_UINT8 = 5U,
    SHADER_UNIFORM_TYPE_INT16 = 6U,
    SHADER_UNIFORM_TYPE_UINT16 = 7U,
    SHADER_UNIFORM_TYPE_INT32 = 8U,
    SHADER_UNIFORM_TYPE_UINT32 = 9U,
    SHADER_UNIFORM_TYPE_MATRIX_4 = 10U,
    SHADER_UNIFORM_TYPE_SAMPLER = 11U,
    SHADER_UNIFORM_TYPE_CUSTOM = 255U
} shader_uniform_type;

// @brief defines the shaders scope, ehich indicates how often it gets updated
typedef enum shader_scope {
    // @brief global shader scope, generally updated once per frame
    SHADER_SCOPE_GLOBAL = 0,
    // @brief instance shader scope, generally updated "per-instance" of the shader
    SHADER_SCOPE_INSTANCE = 1,
    // @brief local shader scope, generally updated per object
    SHADER_SCOPE_LOCAL = 2
} shader_scope;

// @brief configuration for an attribute
typedef struct shader_attribute_config {
    // @brief the length of the name
    u8 name_length;
    // @brief the name of the attributes
    char* name;
    // @brief the size of the attribute
    u8 size;
    // @brief the type of the attribute
    shader_attribute_type type;

} shader_attribute_config;

// @brief configuration for a uniform
typedef struct shader_uniform_config {
    // @brief the length of the name
    u8 name_length;
    // @brief the name of the uniform
    char* name;
    // @brief the size of the uniform
    u8 size;
    // @brief the location of the uniform
    u32 location;
    // @brief the type of the uniform
    shader_uniform_type type;
    // @brief the scope of the uniform
    shader_scope scope;
} shader_uniform_config;

// @brief configuration for a shader. typically created and destroyed by the shader resource loader,
// and set to the properties found in a .shadercfg resource file
typedef struct shader_config {
    // @brief the name of the shader to be created
    char* name;

    // @brief indicates if the shader uses instance-level uniforms
    b8 use_instances;
    // @brief indicates if the shader uses local-level uniforms
    b8 use_local;

    // @brief the count of attributes
    u8 attribute_count;
    // @brief the collection of attributes. darray
    shader_attribute_config* attributes;

    // @brief the count of uniforms
    u8 uniform_count;
    // @brief the collection of uniforms. darray
    shader_uniform_config* uniforms;

    // @brief the name of the renderpass used by this shader
    char* renderpass_name;

    // @brief the number of stages present in the shader
    u8 stage_count;
    // @brief the collection of stages. darray
    shader_stage* stages;
    // @brief the collection of stage names. must align with stages array. darray.
    char** stage_names;
    // @brief the collection of stage file names to be loaded (one per stage). must align with stages array. darray
    char** stage_filenames;
} shader_config;