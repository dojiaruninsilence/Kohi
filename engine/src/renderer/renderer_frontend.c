#include "renderer_frontend.h"

#include "renderer_backend.h"

#include "core/logger.h"
#include "core/kmemory.h"

// backend render context - going to have only a single backend to start with, keep it simple that may change
static renderer_backend* backend = 0;  // create back end with no value

// initialize the renderer
b8 renderer_initialize(const char* application_name, struct platform_state* plat_state) {
    backend = kallocate(sizeof(renderer_backend), MEMORY_TAG_RENDERER);  // allocate memory for the renderer backend and tag it with renderer

    // TODO: this needs to be made configurable
    renderer_backend_create(RENDERER_BACKEND_TYPE_VULKAN, plat_state, backend);  // create the renderer back end, hard coded to vulcan for now, also takes in the platform state, and a pointer to the backend created above
    backend->frame_number = 0;                                                   // zero out the frame number

    if (!backend->initialize(backend, application_name, plat_state)) {    // call the backend initialize - one of the fuction pointers in renderer types
        KFATAL("Renderer backend failed to initialize. Shutting sown.");  // if it fails log a fatal error
        return false;                                                     // and shut down the renderer initialize - boot out
    }

    return true;
}

// shutdown the renderer
void renderer_shutdown() {
    backend->shutdown(backend);                                     // call backend shut down -- another pointer function from renderer types
    kfree(backend, sizeof(renderer_backend), MEMORY_TAG_RENDERER);  // free the memory allocated to the renderer backend - passing in the approptiate tag. cleans up memory
}

// begin a renderer frame - pass in the delta time
b8 renderer_begin_frame(f32 delta_time) {
    return backend->begin_frame(backend, delta_time);  // call backend begin frame pointer function
}

// end a renderer frame - pass in the delta time
b8 renderer_end_frame(f32 delta_time) {
    b8 result = backend->end_frame(backend, delta_time);  // call backend end frame pointer function
    backend->frame_number++;                              // increment the backend frame number
    return result;
}

// on a renderer resize
void renderer_on_resized(u16 width, u16 height) {
    if (backend) {                                                                        // verify that a renderer backend exists to resize
        backend->resized(backend, width, height);                                         // call the pointer function resized and pass in the backend, and pass through the width and the height
    } else {                                                                              // if no backend
        KWARN("renderer backend does not exist to accept resize: %i %i", width, height);  // throw a warning
    }
}

// where the application calls for all draw calls?
b8 renderer_draw_frame(render_packet* packet) {
    // if the begin frame returned successfully, mid-frame operations may continue
    if (renderer_begin_frame(packet->delta_time)) {  // call backend begin frame pointer function, pass in the delta time from the packet

        // end the frame if this fails, it is likely unrecoverable
        b8 result = renderer_end_frame(packet->delta_time);  // call backend end frame pointer function pointer, pass the delta time from the packet, increment the frame number

        if (!result) {
            KERROR("renderer_end_frame failed. application shutting down...");
            return false;
        }
    }

    return true;
}