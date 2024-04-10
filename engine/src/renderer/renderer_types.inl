#pragma once

#include "defines.h"
#include "math/math_types.h"
#include "resources/resource_types.h"

typedef enum renderer_backend_type {
    RENDERER_BACKEND_TYPE_VULKAN,
    RENDERER_BACKEND_TYPE_OPENGL,
    RENDERER_BACKEND_TYPE_DIRECTX
} renderer_backend_type;

// where the data for geometry to be rendered is stored - and a way to pass the geometry to the renderer
typedef struct geometry_render_data {
    mat4 model;          // model matrix for a batch of geometry
    geometry* geometry;  // hold a pointer to a material
} geometry_render_data;

// enum to pass the renderpass id
typedef enum builtin_renderpass {
    BUILTIN_RENDERPASS_WORLD = 0x01,
    BUILTIN_RENDERPASS_UI = 0x02
} builtin_renderpass;

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
    // update the global world state
    void (*update_global_world_state)(mat4 projection, mat4 view, vec3 view_position, vec4 ambient_colour, i32 mode);
    // update the global ui state
    void (*update_global_ui_state)(mat4 projection, mat4 view, i32 mode);

    // end frame cleans everything up for the next frame
    b8 (*end_frame)(struct renderer_backend* backend, f32 delta_time);  // boolean, make sure frame ends succeffully. takes in delta time as well as the backend

    // begin a renderpass
    b8 (*begin_renderpass)(struct renderer_backend* backend, u8 renderpass_id);
    b8 (*end_renderpass)(struct renderer_backend* backend, u8 rederpass_id);

    // update an object using push constants - just pass in a model to upload
    void (*draw_geometry)(geometry_render_data data);  // changed the name of this you have to make sure that everything down form it is changed as well

    // textures

    // create a texture, pass in a name, is it realeased automatically, the size, how many channels it hase,
    // a pointer to the pixels in a u8 array, that is 8 bits per pixel, does it need transparency, and an address for the texture struct
    void (*create_texture)(const u8* pixels, struct texture* texture);

    // destroy a texture
    void (*destroy_texture)(struct texture* texture);

    // materials
    b8 (*create_material)(struct material* material);
    void (*destroy_material)(struct material* material);

    // geometry
    b8 (*create_geometry)(geometry* geometry, u32 vertex_size, u32 vertex_count, const void* vertices, u32 index_size, u32 index_count, const void* indices);
    void (*destroy_geometry)(geometry* geometry);

} renderer_backend;

typedef struct render_packet {
    f32 delta_time;

    u32 geometry_count;                // number of geometries in the array
    geometry_render_data* geometries;  // a pointer to an array of geometries

    u32 ui_geometry_count;                // number of geometries in the array for the ui
    geometry_render_data* ui_geometries;  // a pointer to an array of geometries for the ui
} render_packet;