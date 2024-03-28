#pragma once

#include "math/math_types.h"

// where we will store data for textures - interface
typedef struct texture {
    u32 id;               // id of the texture
    u32 width;            // width of the texture
    u32 height;           // height of the texture
    u8 channel_count;     // how many channels it has (rgba channels)
    b8 has_transparency;  // bool to store whether it has transparencies that need to be considered. alpha channel or not
    u32 generation;
    void* internal_data;  // graphics api specific data
} texture;