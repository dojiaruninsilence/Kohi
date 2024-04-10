#include "renderer_backend.h"

#include "vulkan/vulkan_backend.h"

// create the renderer back end -
b8 renderer_backend_create(renderer_backend_type type, renderer_backend* out_renderer_backend) {
    if (type == RENDERER_BACKEND_TYPE_VULKAN) {                                                       // if using the vulkan backend then set the pointer funtions to these
        out_renderer_backend->initialize = vulkan_renderer_backend_initialize;                        // pointer to the initialize function
        out_renderer_backend->shutdown = vulkan_renderer_backend_shutdown;                            // pointer to the shutdown function
        out_renderer_backend->begin_frame = vulkan_renderer_backend_begin_frame;                      // pointer to the begin frame function
        out_renderer_backend->update_global_world_state = vulkan_renderer_update_global_world_state;  // pointer to the update global state function
        out_renderer_backend->update_global_ui_state = vulkan_renderer_update_global_ui_state;        // pointer to the update global state function
        out_renderer_backend->end_frame = vulkan_renderer_backend_end_frame;                          // pointer to the end frame function
        out_renderer_backend->begin_renderpass = vulkan_renderer_begin_renderpass;
        out_renderer_backend->end_renderpass = vulkan_renderer_end_renderpass;
        out_renderer_backend->resized = vulkan_renderer_backend_on_resized;         // pointer to the on resized function
        out_renderer_backend->draw_geometry = vulkan_renderer_draw_geometry;        // pointer to the update object function
        out_renderer_backend->create_texture = vulkan_renderer_create_texture;      // pointer to the create texture function
        out_renderer_backend->destroy_texture = vulkan_renderer_destroy_texture;    // pointer to the destroy texture function
        out_renderer_backend->create_material = vulkan_renderer_create_material;    // pointer to the create material function
        out_renderer_backend->destroy_material = vulkan_renderer_destroy_material;  // pointer to the destroy material function
        out_renderer_backend->create_geometry = vulkan_renderer_create_geometry;    // pointer to the create geometry function
        out_renderer_backend->destroy_geometry = vulkan_renderer_destroy_geometry;  // pointer to the destroy geometry function

        return true;
    }

    return false;
}

// zeros out all the data in the renderer backend - all of the function pointers
void renderer_backend_destroy(renderer_backend* renderer_backend) {
    renderer_backend->initialize = 0;
    renderer_backend->shutdown = 0;
    renderer_backend->begin_frame = 0;
    renderer_backend->update_global_world_state = 0;
    renderer_backend->update_global_ui_state = 0;
    renderer_backend->end_frame = 0;
    renderer_backend->begin_renderpass = 0;
    renderer_backend->end_renderpass = 0;
    renderer_backend->resized = 0;
    renderer_backend->draw_geometry = 0;
    renderer_backend->create_texture = 0;
    renderer_backend->destroy_texture = 0;
    renderer_backend->create_material = 0;
    renderer_backend->destroy_material = 0;
    renderer_backend->create_geometry = 0;
    renderer_backend->destroy_geometry = 0;
}