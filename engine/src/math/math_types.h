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
typedef struct vec3_u {
    union {
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
    };
} vec3;

// vector 3 union  - can input a combination of any of these below - is a union of types. they all take the same amount of space up
typedef union vec4_u {
#if defined(KUSE_SIMD)  // not going to be using this for now, need to learn what simd is - single instruction multiple data
    // used for SIMD operations
    alignas(16) __m128 data;
#endif
    // an array of x, y, z, w
    alignas(16) f32 elements[4];  // create an aligned array of elements
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

typedef vec4 quat;
