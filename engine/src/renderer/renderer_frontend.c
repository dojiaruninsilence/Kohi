#include "renderer_frontend.h"

#include "renderer_backend.h"

#include "core/logger.h"
#include "core/kmemory.h"
#include "math/kmath.h"

#include "resources/resource_types.h"
#include "systems/texture_system.h"
#include "systems/material_system.h"

// TODO: temporary
#include "core/kstring.h"
#include "core/event.h"
// TODO: end temporary

// where we're going to store all of the info for the renderer systems state
typedef struct renderer_system_state {
    renderer_backend backend;  // store the renderer backend
    mat4 projection;           // store the projection matrix
    mat4 view;                 // store a calculated view matrix
    f32 near_clip;             // hang on to the near_clip value
    f32 far_clip;              // hang on to the far_clip value
} renderer_system_state;

// hold a pointer to the renderer systen state internally
static renderer_system_state* state_ptr;

// initialize the renderer subsystem, - always call twice - on first pass pass in the memory requirement to get the memory required, and zero for the state
// on the second pass - pass in the state as well as the memory rewuirement and actually initialize the subsystem, also pass in a pointer to the application name
b8 renderer_system_initialize(u64* memory_requirement, void* state, const char* application_name) {
    *memory_requirement = sizeof(renderer_system_state);  // de reference the memory requirement and set to the size of the renderer system state
    if (state == 0) {                                     // if no state was passed in
        return true;                                      // boot out here
    }
    state_ptr = state;  // pass through the pointer to the state

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

    return true;
}

// shutdown the renderer
void renderer_system_shutdown(void* state) {
    if (state_ptr) {
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

        u32 count = packet->geometry_count;
        for (u32 i = 0; i < count; ++i) {
            state_ptr->backend.draw_geometry(packet->geometries[i]);
        }

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
void renderer_create_texture(const u8* pixels, struct texture* texture) {
    state_ptr->backend.create_texture(pixels, texture);
}

// just pass through - destroy texture
void renderer_destroy_texture(struct texture* texture) {
    state_ptr->backend.destroy_texture(texture);
}

// materials
b8 renderer_create_material(struct material* material) {
    return state_ptr->backend.create_material(material);
}

void renderer_destroy_material(struct material* material) {
    state_ptr->backend.destroy_material(material);
}

// geometry
b8 renderer_create_geometry(geometry* geometry, u32 vertex_count, const vertex_3d* vertices, u32 index_count, const u32* indices) {
    return state_ptr->backend.create_geometry(geometry, vertex_count, vertices, index_count, indices);
}

void renderer_destroy_geometry(geometry* geometry) {
    state_ptr->backend.destroy_geometry(geometry);
}