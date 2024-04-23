#pragma once

#include "defines.h"
#include "math/math_types.h"
#include "resources/resource_types.h"

#define BUILTIN_SHADER_NAME_MATERIAL "Shader.Builtin.Material"
#define BUILTIN_SHADER_NAME_UI "Shader.Builtin.UI"

struct shader;
struct shader_uniform;

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

typedef enum renderer_debug_view_mode {
    RENDERER_VIEW_MODE_DEFAULT = 0,
    RENDERER_VIEW_MODE_LIGHTING = 1,
    RENDERER_VIEW_MODE_NORMALS = 2
} renderer_debug_view_mode;

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
    void (*texture_create)(const u8* pixels, struct texture* texture);

    // destroy a texture
    void (*texture_destroy)(struct texture* texture);

    // @brief creates a new writeable texture with no data written to it.
    // @param t a pointer to the texture to hold the resources
    void (*texture_create_writeable)(texture* t);

    // @brief resizes a texture. there is no check at this level to see if th e texture is writeable. internal resources are destroyedj
    // and re created at the new resolution. data is lost and would need to be reloaded
    // @param t a pointer to the texture to be resized
    // @param new_width the new width in pixels.
    // @param new_height the new height in pixels
    void (*texture_resize)(texture* t, u32 new_width, u32 new_height);

    // @brief writes the given data to the provided texture NOTE: at this level, this can either be writeable
    // or non writeable texture because this also handles the initial texture load. the texture system itself
    // should be responsible for blocking write requests for non writeable textures
    // @param t a pointer to the texture to be written to
    // @param offset the offset in bytes from the beginning of the data to be written
    // @param size the number of bytes to be written
    // @param pixels the raw image data to be written
    void (*texture_write_data)(texture* t, u32 offset, u32 sizes, const u8* pixels);

    // geometry
    b8 (*create_geometry)(geometry* geometry, u32 vertex_size, u32 vertex_count, const void* vertices, u32 index_size, u32 index_count, const void* indices);
    void (*destroy_geometry)(geometry* geometry);

    // @brief creates internal shader resources using the provided parameters
    // @param s a pointer to the shader
    // @param renderpass_id the identifier of the render pass to be associated with the shader
    // @param stage_count the total number of stages
    // @param stage_filenames an array of shader stage filenames to be loaded. should align with stages array
    // @param stages an array of shader stages indicating what render stages (vert, frag, ect) used in this shader
    // @return b8 true on success, otherwise false
    b8 (*shader_create)(struct shader* shader, u8 renderpass_id, u8 stage_count, const char** stage_filenames, shader_stage* stages);

    // @brief destroys the given shader and releases any resources held by it
    // @param s a pointer to the shader to be destroyed
    void (*shader_destroy)(struct shader* shader);

    // @brief initializes a configured shader. will be automatically destroyed if this step fails
    // must be done after vulkan_shader_create()
    // @param s a pointer to the shader to be initialized
    // @return b8 true on success, otherwise false
    b8 (*shader_initialize)(struct shader* shader);

    // @brief uses the given shader, activating it for updates to attributes, uniforms and such, and for use in draw calls
    // @param s a pointer to the shader to be used
    // @return b8 true on success, otherwise false
    b8 (*shader_use)(struct shader* shader);

    // @brief binds global resources for use and updating
    // @param s a pointer to the shader whose globals are to be bound
    // @return b8 true on success, otherwise false
    b8 (*shader_bind_globals)(struct shader* s);

    // @brief binds instance resources for use and updating
    // @param s a pointer to the shader whose instance resources are to be found
    // @param instance_id the identifier of the instance to be found
    // @return b8 true on success, otherwise false
    b8 (*shader_bind_instance)(struct shader* s, u32 instance_id);

    // @brief applies global data to the uniform buffer
    // @param s a pointer to the shader to apply the global data for
    // @return b8 true on success, otherwise false
    b8 (*shader_apply_globals)(struct shader* s);

    // @brief applies data for the currently bound instances
    // @param s a pointer to the shader to apply the instance data for
    // @param needs_update indicates if the shader uniforms need to be updated or just bound
    // @return b8 true on success, otherwise false
    b8 (*shader_apply_instance)(struct shader* s, b8 needs_update);

    // @brief acquires internal instance level resources and provides an instance id
    // @param s a pointer to the shader to acquire resources from
    // @param maps and array of pointers to texture maps. must be one map per instance texture
    // @param out_instance_id a pointer to hold the new instance identifier
    // @return b8 true on success, otherwise false
    b8 (*shader_acquire_instance_resources)(struct shader* s, texture_map** maps, u32* out_instance_id);

    // @brief releases internal instance level resources for the given instance id
    // @param s a pointer to the shader to release resources from
    // @param instance_id the instance identifier whose resources are to be released
    // @return b8 true on success, otherwise false
    b8 (*shader_release_instance_resources)(struct shader* s, u32 instance_id);

    // @brief sets the uniform of the given shader to the provided value
    // @param s a pointer to the shader
    // @param uniform a constant pointer to the uniform
    // @param value a pointer to the value to be set
    // @return b8 true on success, otherwise false
    b8 (*shader_set_uniform)(struct shader* frontend_shader, struct shader_uniform* uniform, const void* value);

    // @brief acquires internal resources for the given texture map
    // @param map a pointer to the texture map to obtain resources for
    // @return true on success, otherwise false
    b8 (*texture_map_acquire_resources)(struct texture_map* map);

    // @brief releases internal resources for the given texture map
    // @param map a pointer to the texture map to release resources from
    void (*texture_map_release_resources)(struct texture_map* map);
} renderer_backend;

typedef struct render_packet {
    f32 delta_time;

    u32 geometry_count;                // number of geometries in the array
    geometry_render_data* geometries;  // a pointer to an array of geometries

    u32 ui_geometry_count;                // number of geometries in the array for the ui
    geometry_render_data* ui_geometries;  // a pointer to an array of geometries for the ui
} render_packet;