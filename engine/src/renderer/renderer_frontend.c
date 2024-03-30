#include "renderer_frontend.h"

#include "renderer_backend.h"

#include "core/logger.h"
#include "core/kmemory.h"
#include "math/kmath.h"

#include "resources/resource_types.h"

// TODO: temporary
#include "core/kstring.h"
#include "core/event.h"

#define STB_IMAGE_IMPLEMENTATION
#include "vendor/stb_image.h"
// TODO: end temporary

// where we're going to store all of the info for the renderer systems state
typedef struct renderer_system_state {
    renderer_backend backend;  // store the renderer backend
    mat4 projection;           // store the projection matrix
    mat4 view;                 // store a calculated view matrix
    f32 near_clip;             // hang on to the near_clip value
    f32 far_clip;              // hang on to the far_clip value

    texture default_texture;

    // TODO: temporary
    texture test_diffuse;
    // TODO: end temporary
} renderer_system_state;

// hold a pointer to the renderer systen state internally
static renderer_system_state* state_ptr;

void create_texture(texture* t) {
    kzero_memory(t, sizeof(texture));
    t->generation = INVALID_ID;
}

b8 load_texture(const char* texture_name, texture* t) {
    // TODO: should be able to be located anywhere
    char* format_str = "assets/textures/%s.%s";
    const i32 required_channel_count = 4;    // we require 4 channels, if less we add
    stbi_set_flip_vertically_on_load(true);  // images are technically stored upside down, this rights them
    char full_file_path[512];

    // TODO: try different extentions
    string_format(full_file_path, format_str, texture_name, "png");

    // use a temporary texture to load into
    texture temp_texture;

    // opens the file and loads the data, saves an 8 bit integer array
    u8* data = stbi_load(
        full_file_path,
        (i32*)&temp_texture.width,  // have to convert these 3 u32s to i32s
        (i32*)&temp_texture.height,
        (i32*)&temp_texture.channel_count,  // how many channels image has
        required_channel_count);            // what is required channels, will convert if they are different

    temp_texture.channel_count = required_channel_count;  // if the image has the wrong channel count overwrite it here

    if (data) {
        u32 current_generation = t->generation;  // in case the texture has already been loaded, hang on to the id number
        t->generation = INVALID_ID;

        u64 total_size = temp_texture.width * temp_texture.height * required_channel_count;
        // check for transparency
        b32 has_transparency = false;
        for (u64 i = 0; i < total_size; i += required_channel_count) {  // iterate over all of the pixels(count by number of channels)
            u8 a = data[i + 3];                                         // copy the a channel of the pixel
            if (a < 255) {                                              // if the a channel is less than 255 the pixel is at least partially transparent
                has_transparency = true;
                break;
            }
        }

        if (stbi_failure_reason()) {                                                                       // returns a value if there was a failure, 0 if success
            KWARN("load_texture() failed to load file '%s' : %s", full_file_path, stbi_failure_reason());  // warn out the failure
        }

        // aquire internal texture resources and upload to GPU
        renderer_create_texture(
            texture_name,
            true,
            temp_texture.width,
            temp_texture.height,
            temp_texture.channel_count,
            data,
            has_transparency,
            &temp_texture);

        // take a copy of the old texture
        texture old = *t;  // derefence and save to old

        // assign the temp texture to the pointer
        *t = temp_texture;  // dereference and save the temp texture

        // destroy the old texture - to avoid leaking resources
        renderer_destroy_texture(&old);

        if (current_generation == INVALID_ID) {  // if this is the first one
            t->generation = 0;                   // set generation to 0
        } else {
            t->generation = current_generation + 1;  // set generation to an increment generation by one
        }

        // clean up the data
        stbi_image_free(data);
        return true;
    } else {
        if (stbi_failure_reason()) {
            KWARN("load_texture() failed to load file '%s' : %s", full_file_path, stbi_failure_reason());
        }
        return false;
    }
}

// TODO: temporary
b8 event_on_debug_event(u16 code, void* sender, void* listener_inst, event_context data) {
    const char* names[3] = {
        "cobblestone",
        "paving",
        "paving2"};
    static i8 choice = 2;
    choice++;     // increment
    choice %= 3;  // then mod back to 0. still need to learn this

    // load up the new texture
    load_texture(names[choice], &state_ptr->test_diffuse);
    return true;
}
// TODO: end temporary

// initialize the renderer subsystem, - always call twice - on first pass pass in the memory requirement to get the memory required, and zero for the state
// on the second pass - pass in the state as well as the memory rewuirement and actually initialize the subsystem, also pass in a pointer to the application name
b8 renderer_system_initialize(u64* memory_requirement, void* state, const char* application_name) {
    *memory_requirement = sizeof(renderer_system_state);  // de reference the memory requirement and set to the size of the renderer system state
    if (state == 0) {                                     // if no state was passed in
        return true;                                      // boot out here
    }
    state_ptr = state;  // pass through the pointer to the state

    // TODO: temporary
    event_register(EVENT_CODE_DEBUG0, state_ptr, event_on_debug_event);
    // TODO: end temporary

    // take a pointer to defaolt textures for use in the backend
    state_ptr->backend.default_diffuse = &state_ptr->default_texture;

    // TODO: this needs to be made configurable
    renderer_backend_create(RENDERER_BACKEND_TYPE_VULKAN, &state_ptr->backend);  // create the renderer back end, hard coded to vulcan for now and an address to the backend created above
    state_ptr->backend.frame_number = 0;                                         // zero out the frame number

    if (!state_ptr->backend.initialize(&state_ptr->backend, application_name)) {  // call the backend initialize - one of the fuction pointers in renderer types
        KFATAL("Renderer backend failed to initialize. Shutting sown.");          // if it fails log a fatal error
        return false;                                                             // and shut down the renderer initialize - boot out
    }

    // define the near and far clip
    state_ptr->near_clip = 0.1f;
    state_ptr->far_clip = 1000.0f;
    // define default values for the projection matrix - using a perspective style matrix
    state_ptr->projection = mat4_perspective(deg_to_rad(45.0f), 1280 / 720.0f, state_ptr->near_clip, state_ptr->far_clip);

    // define default values for the view matrix
    state_ptr->view = mat4_translation((vec3){0, 0, 30.0f});
    state_ptr->view = mat4_inverse(state_ptr->view);

    // NOTE: create default texture, a 256x256 blue/white checkerboard pattern
    // this is done in code to eliminate asset dependencies
    KTRACE("Creating default texture...");
    const u32 tex_dimension = 256;                          // size the texture will be in width and height
    const u32 channels = 4;                                 // channels
    const u32 pixel_count = tex_dimension * tex_dimension;  // w * h
    u8 pixels[pixel_count * channels];                      // dimensions * channels
    // u8* pixels = kallocate(sizeof(u8) * pixel_count * bpp, MEMORY_TAG_TEXTURE);
    kset_memory(pixels, 255, sizeof(u8) * pixel_count * channels);  // fill the array with 255 which makes it white

    // each pixel
    for (u64 row = 0; row < tex_dimension; ++row) {      // iterate through all the rows
        for (u64 col = 0; col < tex_dimension; ++col) {  // iterate though all the columns
            u64 index = (row * tex_dimension) + col;
            u64 index_bpp = index * channels;
            if (row % 2) {                      // if the row is even
                if (col % 2) {                  // and the col is even
                    pixels[index_bpp + 0] = 0;  // flip the r channel to zero
                    pixels[index_bpp + 1] = 0;  // flip the g channel to zero
                }
            } else {                            // if the row is odd
                if (!(col % 2)) {               // and the col is odd
                    pixels[index_bpp + 0] = 0;  // flip the r channel to zero
                    pixels[index_bpp + 1] = 0;  // flip the g channel to zero
                }
            }
        }
    }
    // create the texture
    renderer_create_texture(
        "default",      // named default
        false,          // it doesnt auto release
        tex_dimension,  // w
        tex_dimension,  // h
        4,              // channels
        pixels,         // pixels in for pixel data
        false,          // no transparency
        &state_ptr->default_texture);

    // manually set the texture generation to invalid since this is a default texture
    state_ptr->default_texture.generation = INVALID_ID;

    // TODO: load other textures
    create_texture(&state_ptr->test_diffuse);

    return true;
}

// shutdown the renderer
void renderer_system_shutdown(void* state) {
    if (state_ptr) {
        // TODO: temporary
        event_unregister(EVENT_CODE_DEBUG0, state_ptr, event_on_debug_event);
        // TODO: end temporary

        // destroy textures
        renderer_destroy_texture(&state_ptr->default_texture);  // destroy the default texture

        renderer_destroy_texture(&state_ptr->test_diffuse);  // destroy the test texture

        state_ptr->backend.shutdown(&state_ptr->backend);  // call backend shut down -- another pointer function from renderer types
    }
    state_ptr = 0;  // reset the state pinter to zero
}

// begin a renderer frame - pass in the delta time
b8 renderer_begin_frame(f32 delta_time) {
    if (!state_ptr) {
        return false;
    }
    return state_ptr->backend.begin_frame(&state_ptr->backend, delta_time);  // call backend begin frame pointer function
}

// end a renderer frame - pass in the delta time
b8 renderer_end_frame(f32 delta_time) {
    if (!state_ptr) {
        return false;
    }
    b8 result = state_ptr->backend.end_frame(&state_ptr->backend, delta_time);  // call backend end frame pointer function
    state_ptr->backend.frame_number++;                                          // increment the backend frame number
    return result;
}

// on a renderer resize
void renderer_on_resized(u16 width, u16 height) {
    if (state_ptr) {                                                                                                                  // verify that a renderer backend exists to resize
        state_ptr->projection = mat4_perspective(deg_to_rad(45.0f), width / (f32)height, state_ptr->near_clip, state_ptr->far_clip);  // re calculate the perspective matrix
        state_ptr->backend.resized(&state_ptr->backend, width, height);                                                               // call the pointer function resized and pass in the backend, and pass through the width and the height
    } else {                                                                                                                          // if no backend
        KWARN("renderer backend does not exist to accept resize: %i %i", width, height);                                              // throw a warning
    }
}

// where the application calls for all draw calls?
b8 renderer_draw_frame(render_packet* packet) {
    // if the begin frame returned successfully, mid-frame operations may continue
    if (renderer_begin_frame(packet->delta_time)) {  // call backend begin frame pointer function, pass in the delta time from the packet

        // update the global state - just passing in default like values for now, to test it - the projection is being calculated now, and view matrix has default values as well
        state_ptr->backend.update_global_state(state_ptr->projection, state_ptr->view, vec3_zero(), vec4_one(), 0);

        mat4 model = mat4_translation((vec3){0, 0, 0});
        // static f32 angle = 0.0f;
        // angle += 0.001f;
        // quat rotation = quat_from_axis_angle(vec3_forward(), angle, false);
        // mat4 model = quat_to_rotation_matrix(rotation, vec3_zero());
        geometry_render_data data = {};
        data.object_id = 0;  // TODO: actual id
        data.model = model;
        data.textures[0] = &state_ptr->test_diffuse;  // set to the default texture
        state_ptr->backend.update_object(data);

        // end the frame if this fails, it is likely unrecoverable
        b8 result = renderer_end_frame(packet->delta_time);  // call backend end frame pointer function pointer, pass the delta time from the packet, increment the frame number

        if (!result) {
            KERROR("renderer_end_frame failed. application shutting down...");
            return false;
        }
    }

    return true;
}

// just pass through - set view
void renderer_set_view(mat4 view) {
    state_ptr->view = view;
}

// just pass through - create texture
void renderer_create_texture(
    const char* name,
    b8 auto_release,
    i32 width,
    i32 height,
    i32 channel_count,
    const u8* pixels,
    b8 has_transparency,
    struct texture* out_texture) {
    state_ptr->backend.create_texture(name, auto_release, width, height, channel_count, pixels, has_transparency, out_texture);
}

// just pass through - destroy texture
void renderer_destroy_texture(struct texture* texture) {
    state_ptr->backend.destroy_texture(texture);
}