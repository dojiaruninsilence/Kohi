#pragma once

#include "defines.h"
#include "math/math_types.h"
#include "renderer/renderer_types.inl"

// @brief the configuration for the render view system
typedef struct render_view_system_config {
    // @brief the maximum number of views that can be registered with the system
    u16 max_view_count;
} render_view_system_config;

// @brief initializes the render view system. call twice, once to obtain memory requiremen (where state=0)
// and a second time with allocated memory passed to the state
// @param memory_requirement a pointer to hold the memory requirement in bytes
// @param state a block of memory to be used for the state
// @param config configuration for the system
// @return true on success, otherwise false
b8 render_view_system_initialize(u64* memory_requirement, void* state, render_view_system_config config);

// @brief shuts the render view system down
// @param state the block of state memory
void render_view_system_shutdown(void* state);

// @brief creates a new view using the provided config. the new view may then be obtained
// via a call to render_view_system_get
// @param config a constant pointer to the view configuration
// @return true on success, otherwise false
b8 render_view_system_create(const render_view_config* config);

// @brief called when the owner of this view (i.e. the windo) is resized
// @param width the new width in pixels
// @param height the new height in pixels
void render_view_system_on_window_resize(u32 width, u32 height);

// @brief obtains a pointer to a view with the given name
// @param name the name of the view
// @return a pointer to a view if found, otherwise 0
render_view* render_view_system_get(const char* name);

// @brief builds a render view packet using the provided view and meshes
// @param view a pointer to the view to use
// @param data freeform data used to build the packet
// @param out_packet a pointer to hold the generated packet
// @return true on success, otherwise false
b8 render_view_system_build_packet(const render_view* view, void* data, struct render_view_packet* out_packet);

// @brief uses the given view and packet to render the contents therein
// @param view a pointer to the view to use
// @param packet a pointer to the packet whose data is to be rendered
// @param frame_number the current renderer frame number, typically used for data syncronization
// @param render_target_index the current render target index for renderers that use multiple render target at once (i.e. vulkan)
// @return true on success, otherwise false
b8 render_view_system_on_render(const render_view* view, const render_view_packet* packet, u64 frame_number, u64 render_target_index);