#pragma once

#include "math/math_types.h"

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
    TEXTURE_USE_MAP_DIFFUSE = 0x01  // textures with this tag will be used as a diffuse map
} texture_use;

// store data for texture maps
typedef struct texture_map {
    texture* texture;
    texture_use use;
} texture_map;

// define the max length that a material may be named
#define MATERIAL_NAME_MAX_LENGTH 256

// where we will store the data for the materials
typedef struct material {
    u32 id;                               // the id of the material
    u32 generation;                       // update whenever the material changes
    u32 internal_id;                      // id handle to the internal material data
    char name[MATERIAL_NAME_MAX_LENGTH];  // name of the material
    vec4 diffuse_colour;
    texture_map diffuse_map;  // store array of textures and their uses
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