#include "renderer_frontend.h"

#include "renderer_backend.h"

#include "core/logger.h"
#include "core/kmemory.h"
#include "math/kmath.h"

#include "resources/resource_types.h"
#include "systems/resource_system.h"
#include "systems/texture_system.h"
#include "systems/material_system.h"
#include "systems/shader_system.h"

// TODO: temporary
#include "core/kstring.h"
#include "core/event.h"
// TODO: end temporary

// where we're going to store all of the info for the renderer systems state
typedef struct renderer_system_state {
    renderer_backend backend;  // store the renderer backend
    mat4 projection;           // store the projection matrix
    mat4 view;                 // store a calculated view matrix
    vec4 ambient_colour;
    vec3 view_position;
    mat4 ui_projection;  // store the projection matrix for the ui
    mat4 ui_view;        // store a calculated view matrix for the ui
    f32 near_clip;       // hang on to the near_clip value
    f32 far_clip;        // hang on to the far_clip value
    u32 material_shader_id;
    u32 ui_shader_id;
    u32 render_mode;
} renderer_system_state;

b8 renderer_on_event(u16 code, void* sender, void* listener_inst, event_context context) {
    switch (code) {
        case EVENT_CODE_SET_RENDER_MODE: {
            renderer_system_state* state = (renderer_system_state*)listener_inst;
            i32 mode = context.data.i32[0];
            switch (mode) {
                default:
                case RENDERER_VIEW_MODE_DEFAULT:
                    KDEBUG("Renderer mode set to default.");
                    state->render_mode = RENDERER_VIEW_MODE_DEFAULT;
                    break;
                case RENDERER_VIEW_MODE_LIGHTING:
                    KDEBUG("Renderer mode set to lighting.");
                    state->render_mode = RENDERER_VIEW_MODE_LIGHTING;
                    break;
                case RENDERER_VIEW_MODE_NORMALS:
                    KDEBUG("Renderer mode set to normals.");
                    state->render_mode = RENDERER_VIEW_MODE_NORMALS;
                    break;
            }
            return true;
        }
    }

    return false;
}

// hold a pointer to the renderer systen state internally
static renderer_system_state* state_ptr;

#define CRITICAL_INIT(op, msg) \
    if (!op) {                 \
        KERROR(msg);           \
        return false;          \
    }

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
    state_ptr->render_mode = RENDERER_VIEW_MODE_DEFAULT;

    event_register(EVENT_CODE_SET_RENDER_MODE, state, renderer_on_event);

    // initialize the backend
    CRITICAL_INIT(state_ptr->backend.initialize(&state_ptr->backend, application_name), "Renderer backend failed to initialize. Shutting down.");

    // shaders
    resource config_resource;
    shader_config* config = 0;

    // builtin material shader
    CRITICAL_INIT(
        resource_system_load(BUILTIN_SHADER_NAME_MATERIAL, RESOURCE_TYPE_SHADER, &config_resource),
        "Failed to load builtin material shader.");
    config = (shader_config*)config_resource.data;
    CRITICAL_INIT(shader_system_create(config), "Failed to load builtin material shader.");
    resource_system_unload(&config_resource);
    state_ptr->material_shader_id = shader_system_get_id(BUILTIN_SHADER_NAME_MATERIAL);

    // builtin ui shader
    CRITICAL_INIT(
        resource_system_load(BUILTIN_SHADER_NAME_UI, RESOURCE_TYPE_SHADER, &config_resource),
        "Failed to load builtin UI shader.");
    config = (shader_config*)config_resource.data;
    CRITICAL_INIT(shader_system_create(config), "Failed to load builtin UI shader.");
    resource_system_unload(&config_resource);
    state_ptr->ui_shader_id = shader_system_get_id(BUILTIN_SHADER_NAME_UI);

    // world projection and view
    // define the near and far clip
    state_ptr->near_clip = 0.1f;
    state_ptr->far_clip = 1000.0f;
    // define default values for the projection matrix - using a perspective style matrix
    state_ptr->projection = mat4_perspective(deg_to_rad(45.0f), 1280 / 720.0f, state_ptr->near_clip, state_ptr->far_clip);

    // TODO: configurable camera starting position
    // define default values for the view matrix
    state_ptr->view = mat4_translation((vec3){0, 0, 30.0f});
    state_ptr->view = mat4_inverse(state_ptr->view);
    // TODO: obtain from the scene
    state_ptr->ambient_colour = (vec4){0.25f, 0.25f, 0.25f, 1.0f};

    // ui projection and view
    state_ptr->ui_projection = mat4_orthographic(0, 1280.0f, 720.0f, 0, -100.0f, 100.0f);  // intentionally flipped on the y axis
    state_ptr->ui_view = mat4_inverse(mat4_identity());

    return true;
}

// shutdown the renderer
void renderer_system_shutdown(void* state) {
    if (state_ptr) {
        state_ptr->backend.shutdown(&state_ptr->backend);  // call backend shut down -- another pointer function from renderer types
    }
    state_ptr = 0;  // reset the state pinter to zero
}

// on a renderer resize
void renderer_on_resized(u16 width, u16 height) {
    if (state_ptr) {                                                                                                                  // verify that a renderer backend exists to resize
        state_ptr->projection = mat4_perspective(deg_to_rad(45.0f), width / (f32)height, state_ptr->near_clip, state_ptr->far_clip);  // re calculate the perspective matrix
        state_ptr->ui_projection = mat4_orthographic(0, (f32)width, (f32)height, 0, -100.0f, 100.0f);                                 // intentionally flipped on the y axis,
        state_ptr->backend.resized(&state_ptr->backend, width, height);                                                               // call the pointer function resized and pass in the backend, and pass through the width and the height
    } else {                                                                                                                          // if no backend
        KWARN("renderer backend does not exist to accept resize: %i %i", width, height);                                              // throw a warning
    }
}

// where the application calls for all draw calls?
b8 renderer_draw_frame(render_packet* packet) {
    state_ptr->backend.frame_number++;
    // if the begin frame returned successfully, mid-frame operations may continue
    if (state_ptr->backend.begin_frame(&state_ptr->backend, packet->delta_time)) {  // call backend begin frame pointer function, pass in the delta time from the packet
        // world renderpass
        if (!state_ptr->backend.begin_renderpass(&state_ptr->backend, BUILTIN_RENDERPASS_WORLD)) {
            KERROR("backend.begin_renderpass -> BUILTIN_RENDERPASS_WORLD failed. Application shutting down...");
            return false;
        }

        if (!shader_system_use_by_id(state_ptr->material_shader_id)) {
            KERROR("Failed to use material shader. Render frame failed.");
            return false;
        }

        // apply globals
        if (!material_system_apply_global(state_ptr->material_shader_id, &state_ptr->projection, &state_ptr->view, &state_ptr->ambient_colour, &state_ptr->view_position, state_ptr->render_mode)) {
            KERROR("Failed to use apply globals for material shader. Render frame failed.");
            return false;
        }

        // draw geometries.
        u32 count = packet->geometry_count;
        for (u32 i = 0; i < count; ++i) {
            material* m = 0;
            if (packet->geometries[i].geometry->material) {
                m = packet->geometries[i].geometry->material;
            } else {
                m = material_system_get_default();
            }

            // apply the material if it hasnt already been this frame. this keeps the same material from being updated many times
            b8 needs_update = m->render_frame_number != state_ptr->backend.frame_number;
            if (!material_system_apply_instance(m, needs_update)) {
                KWARN("Failed to apply material '%s'. Skipping draw.", m->name);
                continue;
            } else {
                // sync the frame number
                m->render_frame_number = state_ptr->backend.frame_number;
            }

            // apply the locals
            material_system_apply_local(m, &packet->geometries[i].model);

            // draw it
            state_ptr->backend.draw_geometry(packet->geometries[i]);
        }

        // end the world renderpass
        if (!state_ptr->backend.end_renderpass(&state_ptr->backend, BUILTIN_RENDERPASS_WORLD)) {
            KERROR("backend.end_renderpass -> BUILTIN_RENDERPASS_WORLD failed. Application shutting down...");
            return false;
        }

        // ui renderpass
        if (!state_ptr->backend.begin_renderpass(&state_ptr->backend, BUILTIN_RENDERPASS_UI)) {
            KERROR("backend.begin_renderpass -> BUILTIN_RENDERPASS_UI failed. Application shutting down...");
            return false;
        }

        if (!shader_system_use_by_id(state_ptr->ui_shader_id)) {
            KERROR("Failed to use UI shader. Render frame failed.");
            return false;
        }

        // apply globals
        if (!material_system_apply_global(state_ptr->ui_shader_id, &state_ptr->ui_projection, &state_ptr->ui_view, 0, 0, 0)) {
            KERROR("Failed to use apply globals for UI shader. Render frame failed.");
            return false;
        }

        // draw ui geometries.
        count = packet->ui_geometry_count;
        for (u32 i = 0; i < count; ++i) {
            material* m = 0;
            if (packet->ui_geometries[i].geometry->material) {
                m = packet->ui_geometries[i].geometry->material;
            } else {
                m = material_system_get_default();
            }

            // apply the material
            b8 needs_update = m->render_frame_number != state_ptr->backend.frame_number;
            if (!material_system_apply_instance(m, needs_update)) {
                KWARN("Failed to apply UI material '%s'. Skipping draw.", m->name);
                continue;
            } else {
                // Sync the frame number.
                m->render_frame_number = state_ptr->backend.frame_number;
            }

            // apply the locals
            material_system_apply_local(m, &packet->ui_geometries[i].model);

            // draw it
            state_ptr->backend.draw_geometry(packet->ui_geometries[i]);
        }

        // end the ui renderpass
        if (!state_ptr->backend.end_renderpass(&state_ptr->backend, BUILTIN_RENDERPASS_UI)) {
            KERROR("backend.end_renderpass -> BUILTIN_RENDERPASS_UI failed. Application shutting down...");
            return false;
        }

        // end the frame if this fails, it is likely unrecoverable
        b8 result = state_ptr->backend.end_frame(&state_ptr->backend, packet->delta_time);

        if (!result) {
            KERROR("renderer_end_frame failed. application shutting down...");
            return false;
        }
    }

    return true;
}

// just pass through - set view
void renderer_set_view(mat4 view, vec3 view_position) {
    state_ptr->view = view;
    state_ptr->view_position = view_position;
}

// just pass through - create texture
void renderer_create_texture(const u8* pixels, struct texture* texture) {
    state_ptr->backend.create_texture(pixels, texture);
}

// just pass through - destroy texture
void renderer_destroy_texture(struct texture* texture) {
    state_ptr->backend.destroy_texture(texture);
}

// geometry
b8 renderer_create_geometry(geometry* geometry, u32 vertex_size, u32 vertex_count, const void* vertices, u32 index_size, u32 index_count, const void* indices) {
    return state_ptr->backend.create_geometry(geometry, vertex_size, vertex_count, vertices, index_size, index_count, indices);
}

void renderer_destroy_geometry(geometry* geometry) {
    state_ptr->backend.destroy_geometry(geometry);
}

b8 renderer_renderpass_id(const char* name, u8* out_renderpass_id) {
    // TODO: HACK: need dynamic renderpasses instead of hardcoding them.
    if (strings_equali("Renderpass.Builtin.World", name)) {
        *out_renderpass_id = BUILTIN_RENDERPASS_WORLD;
        return true;
    } else if (strings_equali("Renderpass.Builtin.UI", name)) {
        *out_renderpass_id = BUILTIN_RENDERPASS_UI;
        return true;
    }

    KERROR("renderer_renderpass_id: No renderpass named '%s'.", name);
    *out_renderpass_id = INVALID_ID_U8;
    return false;
}

b8 renderer_shader_create(shader* s, u8 renderpass_id, u8 stage_count, const char** stage_filenames, shader_stage* stages) {
    return state_ptr->backend.shader_create(s, renderpass_id, stage_count, stage_filenames, stages);
}

void renderer_shader_destroy(shader* s) {
    state_ptr->backend.shader_destroy(s);
}

b8 renderer_shader_initialize(shader* s) {
    return state_ptr->backend.shader_initialize(s);
}

b8 renderer_shader_use(shader* s) {
    return state_ptr->backend.shader_use(s);
}

b8 renderer_shader_bind_globals(shader* s) {
    return state_ptr->backend.shader_bind_globals(s);
}

b8 renderer_shader_bind_instance(shader* s, u32 instance_id) {
    return state_ptr->backend.shader_bind_instance(s, instance_id);
}

b8 renderer_shader_apply_globals(shader* s) {
    return state_ptr->backend.shader_apply_globals(s);
}

b8 renderer_shader_apply_instance(shader* s, b8 needs_updated) {
    return state_ptr->backend.shader_apply_instance(s, needs_updated);
}

b8 renderer_shader_acquire_instance_resources(shader* s, u32* out_instance_id) {
    return state_ptr->backend.shader_acquire_instance_resources(s, out_instance_id);
}

b8 renderer_shader_release_instance_resources(shader* s, u32 instance_id) {
    return state_ptr->backend.shader_release_instance_resources(s, instance_id);
}

b8 renderer_set_uniform(shader* s, shader_uniform* uniform, const void* value) {
    return state_ptr->backend.shader_set_uniform(s, uniform, value);
}