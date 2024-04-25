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

typedef enum renderer_debug_view_mode {
    RENDERER_VIEW_MODE_DEFAULT = 0,
    RENDERER_VIEW_MODE_LIGHTING = 1,
    RENDERER_VIEW_MODE_NORMALS = 2
} renderer_debug_view_mode;

// @brief represents a render target, which is used for rendering to a texture or set of textures
typedef struct render_target {
    // @brief indicates if this render target should be updated on window resize
    b8 sync_to_window_size;
    // @brief the number of attachments
    u8 attachment_count;
    // @brief an array of attachments (pointers to textures)
    struct texture** attachments;
    // @brief the renderer api internal framebuffer object
    void* internal_framebuffer;
} render_target;

// @brief the types of clearing to be done on a renderpass. can be combined together for  multiple clearing functions
typedef enum renderpass_clear_flag {
    // brief No clearing should be done
    RENDERPASS_CLEAR_NONE_FLAG = 0x0,
    // brief clear the color buffer
    RENDERPASS_CLEAR_COLOUR_BUFFER_FLAG = 0x1,
    // brief clear the depth buffer
    RENDERPASS_CLEAR_DEPTH_BUFFER_FLAG = 0x2,
    // brief clear the stencil buffer
    RENDERPASS_CLEAR_STENCIL_BUFFER_FLAG = 0x4,
} renderpass_clear_flag;

typedef struct renderpass_config {
    // @brief the name of this renderpass
    const char* name;
    // @brief the name of the previous renderpass
    const char* prev_name;
    // @brief the name of the next renderpass
    const char* next_name;
    // @brief the current render area of the render pass
    vec4 render_area;
    // @brief the clear color used for this renderpass
    vec4 clear_colour;

    // @brief the clear flags for this render pass
    u8 clear_flags;
} renderpass_config;

// @brief represents a generic renderpass
typedef struct renderpass {
    // @brief the id of the renderpass
    u16 id;

    // @brief the current render area of the renderpass
    vec4 render_area;
    // @brief the clear color used for this renderpass
    vec4 clear_colour;

    // @brief the clear flags for this renderpass
    u8 clear_flags;
    // @brief the number of targets for this renderpass
    u8 render_target_count;
    // @brief an array for render targets used by this renderpass
    render_target* targets;

    // @brief internal renderpass data
    void* internal_data;
} renderpass;

// @brief the generic configuration for a renderer backend
typedef struct renderer_backend_config {
    // @brief the name of the application
    const char* application_name;
    // @brief the number of pointers to the renderpass
    u16 renderpass_count;
    // @brief an array of configurations for renderpassed. will be initialized in the backen automatically
    renderpass_config* pass_configs;
    // @brief a callback that will be made when the backend requires a refresh/regeneration of the render targets
    void (*on_rendertarget_refresh_required)();
} renderer_backend_config;

// one of the places object oriented programing makes sense
// represents the renderer backend
typedef struct renderer_backend {
    u64 frame_number;  // keep track of the frames rendered

    // pointer functions, so we can eventually have multiple backends and not have to change all the code higher level than here

    // @brief initializes the backend
    // @param backend a pointer to the generic backend interface
    // @param config a pointer to configuration to be used when initializing the backend
    // @param out_window_render_target_count a pointer to hold how many render targets are needed for renderpasses targeting the window
    // @return true if initialized successfully, otherwise false
    b8 (*initialize)(struct renderer_backend* backend, const renderer_backend_config* config, u8* out_window_render_target_count);

    void (*shutdown)(struct renderer_backend* backend);  // function ptr shutdown, takes in the pointer to the renderer backend

    void (*resized)(struct renderer_backend* backend, u16 width, u16 height);  // function ptr resized to handle resizing, takes in a ptr the renderer backend and a width and height

    // gets all the stuff ready to render
    b8 (*begin_frame)(struct renderer_backend* backend, f32 delta_time);  // boolean, make sure frame begins succeffully. takes in delta time as well as the backend

    // rendering occurs inbetween

    // end frame cleans everything up for the next frame
    b8 (*end_frame)(struct renderer_backend* backend, f32 delta_time);  // boolean, make sure frame ends succeffully. takes in delta time as well as the backend

    // @brief begins a renderpass with the given id
    // @param pass a pointer to the renderpass to begin
    // @param target a pointer to the render target to be used
    // @return true if successful, otherwise false
    b8 (*renderpass_begin)(renderpass* pass, render_target* target);

    // @brief ends a render pass with the given id
    // @param pass a pointer to the renderpass to end
    // @return true if successful, otherwise false
    b8 (*renderpass_end)(renderpass* pass);

    // @brief obtains a pointer to a renderpass using the provided name
    // @param name the renderpass name
    // @return a pointer to a renderpass, if found, otherwise 0
    renderpass* (*renderpass_get)(const char* name);

    // @brief draws the given geometry. should only be called inside a renderpass, within a frame
    // @param data a pointer to the render data of the geometry to be drawn
    void (*draw_geometry)(geometry_render_data* data);

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
    // @param pass a pointer to the render pass to be associated with the shader
    // @param stage_count the total number of stages
    // @param stage_filenames an array of shader stage filenames to be loaded. should align with stages array
    // @param stages an array of shader stages indicating what render stages (vert, frag, ect) used in this shader
    // @return b8 true on success, otherwise false
    b8 (*shader_create)(struct shader* shader, renderpass* pass, u8 stage_count, const char** stage_filenames, shader_stage* stages);

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

    // @brief creates a new render target using the provided data
    // @param attachment_count the number of attachements (texture pointers)
    // @param attachments an array of attachments (texture pointers)
    // @param renderpass a pointer to the renderpass the render target is associated with
    // @param width the width of the render target in pixels
    // @param height the height of the render target in pixels
    // @param out_target a pointer to hold the newly created render targets
    void (*render_target_create)(u8 attachment_count, texture** attachments, renderpass* pass, u32 width, u32 height, render_target* out_target);

    // @brief destroys the provided render target
    // @param target a pointer to the render target to be destroyed
    // @param free_internal_memory indicates if internal memory should be freed
    void (*render_target_destroy)(render_target* target, b8 free_internal_memory);

    // @brief creates a new renderpass
    // @param out_renderpass a pointer to the generic renderpass
    // @param depth the depth clear amount
    // @param stencil the sencil clear value
    // @param clear_flags the combined clear flags indicating what kind of clear should take place
    // @param has_prev_pass indicates if there is a previous renderpass
    // @param has_next_pass indicates if there is a next renderpass
    void (*renderpass_create)(renderpass* out_renderpass, f32 depth, u32 stencil, b8 has_prev_pass, b8 has_next_pass);

    // @brief destroys the given renderpass
    // @param pass a pointer to the renderpass to be destroyed
    void (*renderpass_destroy)(renderpass* pass);

    // @brief attempts to get the window render target at the given index
    // @param index the index of the attachment to get. must be within the range of window render target count
    // @return a pointer to a texture attachement if successful, otherwise 0
    texture* (*window_attachment_get)(u8 index);

    // @brief returns a pointer to the main depth texture target
    texture* (*depth_attachment_get)();

    // @brief returns the current window attachment index
    u8 (*window_attachment_index_get)();
} renderer_backend;

// @brief known render view types, which have logic associated with them
typedef enum render_view_known_type {
    // @brief a view which only renders objects with no transparency
    RENDERER_VIEW_KNOWN_TYPE_WORLD = 0x01,
    // @brief a view which only renders ui objects
    RENDERER_VIEW_KNOWN_TYPE_UI = 0x02
} render_view_known_type;

// @brief known view matrix sources
typedef enum render_view_view_matrix_source {
    RENDER_VIEW_VIEW_MATRIX_SOURCE_SCENE_CAMERA = 0x01,
    RENDER_VIEW_VIEW_MATRIX_SOURCE_UI_CAMERA = 0x02,
    RENDER_VIEW_VIEW_MATRIX_SOURCE_LIGHT_CAMERA = 0x03,
} render_view_view_matrix_source;

// @brief known projection matrix sources
typedef enum render_view_projection_matrix_source {
    RENDER_VIEW_PROJECTION_MATRIX_SOURCE_DEFAULT_PERSPECTIVE = 0x01,
    RENDER_VIEW_PROJECTION_MATRIX_SOURCE_DEFAULT_ORTHOGRAPHIC = 0x02,
} render_view_projection_matrix_source;

// @brief configuration for a renderpass to be associated with a view
typedef struct render_view_pass_config {
    const char* name;
} render_view_pass_config;

// @brief the configuration of a render view. used as a serialization target
typedef struct render_view_config {
    // @brief the name of the view
    const char* name;

    // @brief the name of the custom shader to be used instead of the view's
    // default. must be 0 if not used
    const char* custom_shader_name;
    // @brief the width of the view. set to 0 for 100% width
    u16 width;
    // @brief the height of the view. set to 0 for 100% height
    u16 height;
    // @brief the known type of the view. used to be associate with view logic
    render_view_known_type type;
    // @brief the source of the view matrix
    render_view_view_matrix_source view_matrix_source;
    // @brief the source of the view matrix
    render_view_projection_matrix_source projection_matrix_source;
    // @brief the number of renderpasses used in this view
    u8 pass_count;
    // @brief the number of renderpasses used in this view
    render_view_pass_config* passes;
} render_view_config;

struct render_view_packet;

// @breif a render view instance, responsible for the generation of view packets
// based on internal logic and the given config
typedef struct render_view {
    // @brief the unique identifier of this view
    u16 id;
    // @brief the name of the view
    const char* name;
    // @brief the current width of this view
    u16 width;
    // @brief the current height of this view
    u16 height;
    // @brief the known type of this view
    render_view_known_type type;

    // @brief the number of renderpasses used by this view
    u8 renderpass_count;
    // @brief an array of pointers to renderpasses used by this view
    renderpass** passes;

    // @brief the name of the custom shader used by this view, if there is one
    const char* custom_shader_name;
    // @brief the internal, view specific data for this view
    void* internal_data;

    // @brief a pointer to a function to be called when this view is created
    // @param self a pointer to the view being created
    // @return true on success, otherwise false
    b8 (*on_create)(struct render_view* self);

    // @brief a pointer to a function to be called when this view is destroyed
    // @param self a pointer to the view being destroyed
    void (*on_destroy)(struct render_view* self);

    // @brief a pointer to a function to be called when the owner fo this view (such as the window) is resized
    // @param self a pointer to the view being resized
    // @param width the new width in pixels
    // @param height the new height in pixels
    void (*on_resize)(struct render_view* self, u32 width, u32 height);

    // @brief builds a render view packet using the provided view and meshes
    // @param self a pointer to the view to use
    // @param data freeform data used to build the packet
    // @param out_packet a pointer to hold the generated packet
    // @return true on success, otherwise false
    b8 (*on_build_packet)(const struct render_view* self, void* data, struct render_view_packet* out_packet);

    // @brief Uses the given view and packet to render the contents therein.
    // @param self a pointer to the view to use
    // @param packet a pointer to the packet whose data is to be rendered
    // @param frame_number the current renderer frame number, typically used for data synchronization
    //  @param render_target_index the current render target index for renderers that use multiple render targets at once (i.e. vulkan)
    // @return true on success, otherwise false
    b8 (*on_render)(const struct render_view* self, const struct render_view_packet* packet, u64 frame_number, u64 render_target_index);
} render_view;

// @brief a packet for and generated by a render view, which contains data about what is to be rendered
typedef struct render_view_packet {
    // @brief a constant pointer to the view this packet is associated with
    const render_view* view;
    // @brief the current view matrix
    mat4 view_matrix;
    // @brief the current projection matrix
    mat4 projection_matrix;
    // @brief the current view position, if applicable
    vec3 view_position;
    // @brief the current scene ambient color if applicable
    vec4 ambient_colour;
    // @brief the number of geometries to be drawn
    u32 geometry_count;
    // @brief the geometries to be drawn
    geometry_render_data* geometries;
    // @brief the name of the custom shader to use, if applicable, otherwise 0
    const char* custom_shader_name;
    // @brief holds a pointer to freeform data, typically understood by the object and consuming view
    void* extended_data;
} render_view_packet;

typedef struct mesh_packet_data {
    u32 mesh_count;
    mesh* meshes;
} mesh_packet_data;

// @brief a structure which is generated by the application and sent once to the renderer to render a given frame.
// consists of any data required, such as delta time and a collection of views to be rendered
typedef struct render_packet {
    f32 delta_time;
    // the number of views to be rendered
    u16 view_count;
    // an array of views to be rendered
    render_view_packet* views;
} render_packet;