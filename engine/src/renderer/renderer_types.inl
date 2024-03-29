#pragma once

#include "defines.h"
#include "math/math_types.h"
#include "resources/resource_types.h"

typedef enum renderer_backend_type {
    RENDERER_BACKEND_TYPE_VULKAN,
    RENDERER_BACKEND_TYPE_OPENGL,
    RENDERER_BACKEND_TYPE_DIRECTX
} renderer_backend_type;

// nvidia has a deal where these have to be 256 bytes perfectly, so its set up like this
// where we are going to hold the data for the global uniforms
typedef struct global_uniform_object {
    mat4 projection;   // store projection matrices - 64bytes
    mat4 view;         // store view matrices       - 64bytes
    mat4 m_reserved0;  // 64 bytes, reserved for future use
    mat4 m_reserved1;  // 64 bytes, reserved for future use
} global_uniform_object;

// this name will change, but for now - like the global ubo, but this one is for each object - so potentially updating every frame for every object
typedef struct object_uniform_object {
    vec4 diffuse_color;  // 16 bytes
    vec4 v_reserved0;    // 16 bytes, reserved for future use
    vec4 v_reserved1;    // 16 bytes, reserved for future use
    vec4 v_reserved2;    // 16 bytes, reserved for future use
} object_uniform_object;

// where the data for geometry to be rendered is stored - and a way to pass the geometry to the renderer
typedef struct geometry_render_data {
    u32 object_id;          // each object going to have an id, so we dont have to hold pointers to everything
    mat4 model;             // model matrix for a batch of geometry
    texture* textures[16];  // can have up to sixteen different texture pointers in an array
} geometry_render_data;

// one of the places object oriented programing makes sense
// represents the renderer backend
typedef struct renderer_backend {
    u64 frame_number;  // keep track of the frames rendered

    // pointer functions, so we can eventually have multiple backends and not have to change all the code higher level than here

    b8 (*initialize)(struct renderer_backend* backend, const char* application_name);  // funtion ptr called initialize, takes in a ptr to a renderer backend, pointer to the application name

    void (*shutdown)(struct renderer_backend* backend);  // function ptr shutdown, takes in the pointer to the renderer backend

    void (*resized)(struct renderer_backend* backend, u16 width, u16 height);  // function ptr resized to handle resizing, takes in a ptr the renderer backend and a width and height

    // gets all the stuff ready to render
    b8 (*begin_frame)(struct renderer_backend* backend, f32 delta_time);  // boolean, make sure frame begins succeffully. takes in delta time as well as the backend

    // rendering occurs inbetween
    // update the global state
    void (*update_global_state)(mat4 projection, mat4 view, vec3 view_position, vec4 ambient_colour, i32 mode);

    // end frame cleans everything up for the next frame
    b8 (*end_frame)(struct renderer_backend* backend, f32 delta_time);  // boolean, make sure frame ends succeffully. takes in delta time as well as the backend

    // update an object using push constants - just pass in a model to upload
    void (*update_object)(geometry_render_data data);

    // textures

    // create a texture, pass in a name, is it realeased automatically, the size, how many channels it hase,
    // a pointer to the pixels in a u8 array, that is 8 bits per pixel, does it need transparency, and an address for the texture struct
    void (*create_texture)(
        const char* name,
        b8 auto_release,
        i32 width,
        i32 height,
        i32 channel_count,
        const u8* pixels,
        b8 has_transparency,
        struct texture* out_texture);

    // destroy a texture
    void (*destroy_texture)(struct texture* texture);

} renderer_backend;

typedef struct render_packet {
    f32 delta_time;
} render_packet;