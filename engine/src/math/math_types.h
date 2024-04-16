#pragma once

#include "defines.h"

// vector 2 union - can input a combination of any of these below - is a union of types. they all take the same amount of space up
typedef union vec2_u {
    // an array of x, y
    f32 elements[2];  // input as an array of elements
    struct {
        union {
            // the first element.
            f32 x, r, s, u;  // input a position, color ect
        };
        union {
            // the second element
            f32 y, g, t, v;  // input a position, color ect
        };
    };
} vec2;

// vector 3 union  - can input a combination of any of these below - is a union of types. they all take the same amount of space up
typedef union vec3_u {
    // an array of x, y, z
    f32 elements[3];  // input as an array of elements
    struct {
        union {
            // the first element
            f32 x, r, s, u;  // input a position, color ect
        };
        union {
            // the second element
            f32 y, g, t, v;  // input a position, color ect
        };
        union {
            // the third element
            f32 z, b, p, w;  // input a position, color ect
        };
    };
} vec3;

// vector 3 union  - can input a combination of any of these below - is a union of types. they all take the same amount of space up
typedef union vec4_u {
    // an array of x, y, z, w
    f32 elements[4];  // create an aligned array of elements
    union {
        struct {
            union {
                // the first element
                f32 x, r, s;
            };
            union {
                // the second element
                f32 y, g, t;
            };
            union {
                // the third element
                f32 z, b, p;
            };
            union {
                // the fourth element
                f32 w, a, q;
            };
        };
    };
} vec4;

typedef vec4 quat;  // can use quat instead of vec4 for clarity sake later on

// mat 4 union
typedef union mat4_u {
    f32 data[16];  // align memory for a 16 element floating point array, call it data - for a 4 by 4 matrix
} mat4;

// vertex is an individual point of geometry, that holds various bits of information, not only position, but texture mappinc coords, color, ect -
// these want to be in this order everywhere the are used
typedef struct vertex_3d {
    vec3 position;  // we are only going to hold the position to start with in a vec3
    // @brief the normal of the vertex - like the direction that is straight out
    vec3 normal;

    vec2 texcoord;
} vertex_3d;

// for 2 dimentional renderering
typedef struct vertex_2d {
    vec2 position;
    vec2 texcoord;
} vertex_2d;