#include "renderer_backend.h"

#include "vulkan/vulkan_backend.h"

// create the renderer back end -
b8 renderer_backend_create(renderer_backend_type type, renderer_backend* out_renderer_backend) {
    if (type == RENDERER_BACKEND_TYPE_VULKAN) {                                           // if using the vulkan backend then set the pointer funtions to these
        out_renderer_backend->initialize = vulkan_renderer_backend_initialize;            // pointer to the initialize function
        out_renderer_backend->shutdown = vulkan_renderer_backend_shutdown;                // pointer to the shutdown function
        out_renderer_backend->begin_frame = vulkan_renderer_backend_begin_frame;          // pointer to the begin frame function
        out_renderer_backend->update_global_state = vulkan_renderer_update_global_state;  // pointer to the update global state function
        out_renderer_backend->end_frame = vulkan_renderer_backend_end_frame;              // pointer to the end frame function
        out_renderer_backend->resized = vulkan_renderer_backend_on_resized;               // pointer to the on resized function
        out_renderer_backend->update_object = vulkan_backend_update_object;               // pointer to the update object function
        out_renderer_backend->create_texture = vulkan_renderer_create_texture;            // pointer to the create texture function
        out_renderer_backend->destroy_texture = vulkan_renderer_destroy_texture;          // pointer to the destroy texture function

        return true;
    }

    return false;
}

// zeros out all the data in the renderer backend - all of the function pointers
void renderer_backend_destroy(renderer_backend* renderer_backend) {
    renderer_backend->initialize = 0;
    renderer_backend->shutdown = 0;
    renderer_backend->begin_frame = 0;
    renderer_backend->update_global_state = 0;
    renderer_backend->end_frame = 0;
    renderer_backend->resized = 0;
    renderer_backend->update_object = 0;
    renderer_backend->create_texture = 0;
    renderer_backend->destroy_texture = 0;
}