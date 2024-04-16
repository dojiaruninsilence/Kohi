#include "geometry_system.h"

#include "core/logger.h"
#include "core/kmemory.h"
#include "core/kstring.h"
#include "systems/material_system.h"
#include "renderer/renderer_frontend.h"

// keep refences to the geometries
typedef struct geometry_reference {
    u64 reference_count;  // count of geometries
    geometry geometry;    // actual geometry
    b8 auto_release;      // does it auto release
} geometry_reference;

// where we store the state of the geometry system
typedef struct geometry_system_state {
    geometry_system_config config;  // store the configurations of the system

    geometry default_geometry;  // store a default geometry
    geometry default_2d_geometry;

    // array of registered meshes
    geometry_reference* registered_geometries;
} geometry_system_state;

// local pointer to the system state
static geometry_system_state* state_ptr = 0;

// forward declarations for local functions
b8 create_default_geometries(geometry_system_state* state);
b8 create_geometry(geometry_system_state* state, geometry_config config, geometry* g);
void destroy_geometry(geometry_system_state* state, geometry* g);

// initialize the geometry system - 2 stage initializatin, first stage is to get the memory requirement to allocate the memory, second call actually initializes it
b8 geometry_system_initialize(u64* memory_requirement, void* state, geometry_system_config config) {
    if (config.max_geometry_count == 0) {                                                    // make sure that there is actually an allowence for geometries
        KFATAL("geometry_system_initialize - config.max_geometry must be greater than 0.");  // throw fatal
        return false;
    }

    // Block of memory will contain state structure, then block for array, then block for hashtable if we decide that we need one
    u64 struct_requirement = sizeof(geometry_system_state);
    u64 array_requirement = sizeof(geometry_reference) * config.max_geometry_count;
    *memory_requirement = struct_requirement + array_requirement;

    // if no state was passed in, just trying to get the memory requirements, boot out here
    if (!state) {
        return true;
    }

    // pass through the values for the state
    state_ptr = state;
    state_ptr->config = config;

    // the array block is after the state.  Already allocated, so just set the pointer
    void* array_block = state + struct_requirement;
    state_ptr->registered_geometries = array_block;

    // invalidate all geometries in the array
    u32 count = state_ptr->config.max_geometry_count;
    for (u32 i = 0; i < count; ++i) {  // iterate through the entire geometry array and pass in an invalid id
        state_ptr->registered_geometries[i].geometry.id = INVALID_ID;
        state_ptr->registered_geometries[i].geometry.internal_id = INVALID_ID;
        state_ptr->registered_geometries[i].geometry.generation = INVALID_ID;
    }

    // create the default geometry - throw fatal if it fails
    if (!create_default_geometries(state_ptr)) {
        KFATAL("Failed to create the default geometries. Application cannot continue.");
        return false;
    }

    // everything a success
    return true;
}

// shutdown the geometry system
void geometry_system_shutdown(void* state) {
    // NOTE: nothing to do here for now.
}

// @brief Aquires and existing geometry by id
// @param id the geometry identifier to acquire by
// @return a pointer to the acquired geometry or nullptr if failed
geometry* geometry_system_acquire_by_id(u32 id) {
    // if the id is not invalid and that the geometry registered at the id index isnt invalid id
    if (id != INVALID_ID && state_ptr->registered_geometries[id].geometry.id != INVALID_ID) {
        state_ptr->registered_geometries[id].reference_count++;  // increment the reference count
        return &state_ptr->registered_geometries[id].geometry;   // return a pointer to the geometry
    }

    // NOTE: should return default geometry instead?
    KERROR("geometry_system_acquire_by_id cannot load invalid geometry id. returning nullptr.");
    return 0;
}

// @brief registers and acquires a new geometry using the given config
// @param config the geometry configuration
// @param auto_release Indicates if the aquired geometry should be unloaded when its refence count reaches 0
// @return a pointer to the acquired geometry or nullptr if failed
geometry* geometry_system_acquire_from_config(geometry_config config, b8 auto_release) {
    geometry* g = 0;                                                  // define a pointer for the geometry
    for (u32 i = 0; i < state_ptr->config.max_geometry_count; ++i) {  // iterate through the array looking for an empty slot
        if (state_ptr->registered_geometries[i].geometry.id == INVALID_ID) {
            // found empty slot
            state_ptr->registered_geometries[i].auto_release = auto_release;
            state_ptr->registered_geometries[i].reference_count = 1;  // initialize the refence count at 1
            g = &state_ptr->registered_geometries[i].geometry;        // attach the pointer to the goemetry
            g->id = i;                                                // set the id to i
            break;
        }
    }

    // if there is nothing in g it failed and bleet an error about it
    if (!g) {
        KERROR("Unable to obtain free slot for geometry. Adjust configuration to allow more space. Returning nullptr.");
        return 0;
    }

    // create the geometry, bleet error if it fails
    if (!create_geometry(state_ptr, config, g)) {
        KERROR("Failed to create geometry. Returning nullptr.");
        return 0;
    }

    return g;
}

// @brief realeases a reference to the provided geometry
// @param geometry the geometry to be released
void geometry_system_release(geometry* geometry) {
    if (geometry && geometry->id != INVALID_ID) {                                   // verify that a geometry was passed in and that it has a valid id
        geometry_reference* ref = &state_ptr->registered_geometries[geometry->id];  // get a pointer to the registered geometry using the id from passed in geometry

        // take a copy of the id
        u32 id = geometry->id;
        if (ref->geometry.id == id) {        // if the passed in id and the registered id match
            if (ref->reference_count > 0) {  // if the ref count is not zero
                ref->reference_count--;      // decrement it
            }

            // also blanks out the geometry id
            if (ref->reference_count < 1 && ref->auto_release) {  // if the ref count is brought to zero, and the geometry is set to auto release
                destroy_geometry(state_ptr, &ref->geometry);      // then destroy the geomety
                ref->reference_count = 0;                         // reset the ref vount
                ref->auto_release = false;                        // and reset the auto release
            }
        } else {  // if the ids dont match
            KFATAL("Geometry id mismatch. check registration logic, as this should never occur.");
        }
        return;
    }
    // if invalid id was passed in
    KWARN("geometry_system_release cannot release invalid geometry id. nothing was done.");
}

// @brief obtains a pointer to the default geometry
// @return a pointer to the default geometry
geometry* geometry_system_get_default() {
    if (state_ptr) {
        return &state_ptr->default_geometry;
    }

    KFATAL("geometry_system_get_default called before the system was initialized. Returning nullptr");
    return 0;
}

geometry* geometry_system_get_default_2d() {
    if (state_ptr) {
        return &state_ptr->default_2d_geometry;
    }

    KFATAL("geometry_system_get_default_2d called before the system was initialized. Returning nullptr.");
    return 0;
}

b8 create_geometry(geometry_system_state* state, geometry_config config, geometry* g) {
    // send the geometry off to the renderer to be uploaded to the GPU
    if (!renderer_create_geometry(g, config.vertex_size, config.vertex_count, config.vertices, config.index_size, config.index_count, config.indices)) {
        // if geometry is failed to create invalidate the entry
        state->registered_geometries[g->id].reference_count = 0;
        state->registered_geometries[g->id].auto_release = false;
        g->id = INVALID_ID;
        g->generation = INVALID_ID;
        g->internal_id = INVALID_ID;

        return false;
    }

    // acquire the material
    if (string_length(config.material_name) > 0) {
        g->material = material_system_acquire(config.material_name);
        if (!g->material) {
            g->material = material_system_get_default();
        }
    }

    return true;
}

void destroy_geometry(geometry_system_state* state, geometry* g) {
    renderer_destroy_geometry(g);
    g->internal_id = INVALID_ID;
    g->generation = INVALID_ID;
    g->id = INVALID_ID;

    string_empty(g->name);

    // release the material
    if (g->material && string_length(g->material->name) > 0) {
        material_system_release(g->material->name);
        g->material = 0;
    }
}

b8 create_default_geometries(geometry_system_state* state) {
    vertex_3d verts[4];
    kzero_memory(verts, sizeof(vertex_3d) * 4);

    // creates a basic square plane
    const f32 f = 10.0f;
    verts[0].position.x = -0.5 * f;  // 0    3
    verts[0].position.y = -0.5 * f;  //
    verts[0].texcoord.x = 0.0;       //
    verts[0].texcoord.y = 0.0;       // 2    1

    verts[1].position.x = 0.5 * f;
    verts[1].position.y = 0.5 * f;
    verts[1].texcoord.x = 1.0;
    verts[1].texcoord.y = 1.0;

    verts[2].position.x = -0.5 * f;
    verts[2].position.y = 0.5 * f;
    verts[2].texcoord.x = 0.0;
    verts[2].texcoord.y = 1.0;

    verts[3].position.x = 0.5 * f;
    verts[3].position.y = -0.5 * f;
    verts[3].texcoord.x = 1.0;
    verts[3].texcoord.y = 0.0;

    u32 indices[6] = {0, 1, 2, 0, 3, 1};

    // send the geomtery off to the renderer to be uploaded to the GPU
    state->default_geometry.internal_id = INVALID_ID;
    if (!renderer_create_geometry(&state->default_geometry, sizeof(vertex_3d), 4, verts, sizeof(u32), 6, indices)) {
        KFATAL("Failed to create the default geometry. Application cannot continue.");
        return false;
    }

    // acquire the default material
    state->default_geometry.material = material_system_get_default();

    vertex_2d verts2d[4];
    kzero_memory(verts2d, sizeof(vertex_2d) * 4);

    // creates a basic square plane
    verts2d[0].position.x = -0.5 * f;  // 0    3
    verts2d[0].position.y = -0.5 * f;  //
    verts2d[0].texcoord.x = 0.0;       //
    verts2d[0].texcoord.y = 0.0;       // 2    1

    verts2d[1].position.x = 0.5 * f;
    verts2d[1].position.y = 0.5 * f;
    verts2d[1].texcoord.x = 1.0;
    verts2d[1].texcoord.y = 1.0;

    verts2d[2].position.x = -0.5 * f;
    verts2d[2].position.y = 0.5 * f;
    verts2d[2].texcoord.x = 0.0;
    verts2d[2].texcoord.y = 1.0;

    verts2d[3].position.x = 0.5 * f;
    verts2d[3].position.y = -0.5 * f;
    verts2d[3].texcoord.x = 1.0;
    verts2d[3].texcoord.y = 0.0;

    // indices (NOTE: counter-clockwise)
    u32 indices2d[6] = {2, 1, 0, 3, 0, 1};

    // send the geomtery off to the renderer to be uploaded to the GPU
    if (!renderer_create_geometry(&state->default_2d_geometry, sizeof(vertex_2d), 4, verts2d, sizeof(u32), 6, indices2d)) {
        KFATAL("Failed to create the default 2d geometry. Application cannot continue.");
        return false;
    }

    // acquire the default material
    state->default_2d_geometry.material = material_system_get_default();

    return true;
}

// @brief Generates configuration for plane geometries given the provided parameters.
// NOTE: vertex and index arrays are dynamically allocated and should be freed upon object disposal.  Thus, this should not be considered production code
// @param width the overall width of the plane. must be non zero
// @param height the overall height of the plane. must be non zero
// @param x_segment_count the number of segments along the x axis in the plane.  must be non-zero
// @param y_segment_count the number of segments along the y axis in the plane.  must be non-zero
// @param tile_x the number of times the texture should tile across the plane on the x axis. must be non zero
// @param tile_y the number of times the texture should tile across the plane on the y axis. must be non zero
// @param name the name of the generated geometry
// @param material_name the name of the material to be used
// @return a geometry configuration which can then be fed into geometry_system_acquire_from_config()
geometry_config geometry_system_generate_plane_config(f32 width, f32 height, u32 x_segment_count, u32 y_segment_count, f32 tile_x, f32 tile_y, const char* name, const char* material_name) {
    // if width and/or height are zero, set them to one
    if (width == 0) {
        KWARN("Width must be nonzero. Defaulting to one.");
        width = 1.0f;
    }
    if (height == 0) {
        KWARN("Height must be nonzero. Defaulting to one.");
        height = 1.0f;
    }

    // if either/both segment counts are zero, set them to one
    if (x_segment_count < 1) {
        KWARN("x_segment_count must be a positive number. Defaulting to one.");
        x_segment_count = 1;
    }
    if (y_segment_count < 1) {
        KWARN("y_segment_count must be a positive number. Defaulting to one.");
        y_segment_count = 1;
    }

    // if either/both tile settings are zero, set them to one
    if (tile_x == 0) {
        KWARN("tile_x must be nonzero. Defaulting to one.");
        tile_x = 1.0f;
    }
    if (tile_y == 0) {
        KWARN("tile_y must be nonzero. Defaulting to one.");
        tile_y = 1.0f;
    }

    geometry_config config;
    config.vertex_size = sizeof(vertex_3d);
    config.vertex_count = x_segment_count * y_segment_count * 4;                             // 4 verts per segment
    config.vertices = kallocate(sizeof(vertex_3d) * config.vertex_count, MEMORY_TAG_ARRAY);  // allocate memory for the vertex array
    config.index_size = sizeof(u32);
    config.index_count = x_segment_count * y_segment_count * 6;                      // 6 indices per segment
    config.indices = kallocate(sizeof(u32) * config.index_count, MEMORY_TAG_ARRAY);  // allocate memory for the index array

    // TODO: this generates extro vertices, but we can always deduplicate them later
    f32 seg_width = width / x_segment_count;    // divide the width by count to get the length of each segment
    f32 seg_height = height / y_segment_count;  // divide the height by count to get the length of each segment
    f32 half_width = width * 0.5f;
    f32 half_height = height * 0.5f;
    for (u32 y = 0; y < y_segment_count; ++y) {      // iterate through the y segment count - the rows
        for (u32 x = 0; x < x_segment_count; ++x) {  // iterate through the x segment count - the collums
            // generate the vertices
            // create centered geometry based on the inputs for each segment
            f32 min_x = (x * seg_width) - half_width;    // this is the left position
            f32 min_y = (y * seg_height) - half_height;  // this is the top postion
            f32 max_x = min_x + seg_width;               // this is the right position
            f32 max_y = min_y + seg_height;              // this is the left position

            // texture coords, with the tiling calculated into it
            f32 min_uvx = (x / (f32)x_segment_count) * tile_x;
            f32 min_uvy = (y / (f32)y_segment_count) * tile_y;
            f32 max_uvx = ((x + 1) / (f32)x_segment_count) * tile_x;
            f32 max_uvy = ((y + 1) / (f32)y_segment_count) * tile_y;

            // calculate the correct place in the array to put the vertex data
            u32 v_offset = ((y * x_segment_count) + x) * 4;
            vertex_3d* v0 = &((vertex_3d*)config.vertices)[v_offset + 0];
            vertex_3d* v1 = &((vertex_3d*)config.vertices)[v_offset + 1];
            vertex_3d* v2 = &((vertex_3d*)config.vertices)[v_offset + 2];
            vertex_3d* v3 = &((vertex_3d*)config.vertices)[v_offset + 3];

            // assign all the vertex points
            v0->position.x = min_x;
            v0->position.y = min_y;
            v0->texcoord.x = min_uvx;
            v0->texcoord.y = min_uvy;

            v1->position.x = max_x;
            v1->position.y = max_y;
            v1->texcoord.x = max_uvx;
            v1->texcoord.y = max_uvy;

            v2->position.x = min_x;
            v2->position.y = max_y;
            v2->texcoord.x = min_uvx;
            v2->texcoord.y = max_uvy;

            v3->position.x = max_x;
            v3->position.y = min_y;
            v3->texcoord.x = max_uvx;
            v3->texcoord.y = min_uvy;

            // generate the indices
            u32 i_offset = ((y * x_segment_count) + x) * 6;
            ((u32*)config.indices)[i_offset + 0] = v_offset + 0;
            ((u32*)config.indices)[i_offset + 1] = v_offset + 1;
            ((u32*)config.indices)[i_offset + 2] = v_offset + 2;
            ((u32*)config.indices)[i_offset + 3] = v_offset + 0;
            ((u32*)config.indices)[i_offset + 4] = v_offset + 3;
            ((u32*)config.indices)[i_offset + 5] = v_offset + 1;
        }
    }

    if (name && string_length(name) > 0) {
        string_ncopy(config.name, name, GEOMETRY_NAME_MAX_LENGTH);
    } else {
        string_ncopy(config.name, DEFAULT_GEOMETRY_NAME, GEOMETRY_NAME_MAX_LENGTH);
    }

    if (material_name && string_length(material_name) > 0) {
        string_ncopy(config.material_name, material_name, MATERIAL_NAME_MAX_LENGTH);
    } else {
        string_ncopy(config.material_name, DEFAULT_MATERIAL_NAME, MATERIAL_NAME_MAX_LENGTH);
    }

    return config;
}

geometry_config geometry_system_generate_cube_config(f32 width, f32 height, f32 depth, f32 tile_x, f32 tile_y, const char* name, const char* material_name) {
    if (width == 0) {
        KWARN("Width must be nonzero. Defaulting to one.");
        width = 1.0f;
    }
    if (height == 0) {
        KWARN("Height must be nonzero. Defaulting to one.");
        height = 1.0f;
    }
    if (depth == 0) {
        KWARN("Depth must be nonzero. Defaulting to one.");
        depth = 1;
    }
    if (tile_x == 0) {
        KWARN("tile_x must be nonzero. Defaulting to one.");
        tile_x = 1.0f;
    }
    if (tile_y == 0) {
        KWARN("tile_y must be nonzero. Defaulting to one.");
        tile_y = 1.0f;
    }

    geometry_config config;
    config.vertex_size = sizeof(vertex_3d);
    config.vertex_count = 4 * 6;  // 4 verts per side, 6 sides
    config.vertices = kallocate(sizeof(vertex_3d) * config.vertex_count, MEMORY_TAG_ARRAY);
    config.index_size = sizeof(u32);
    config.index_count = 6 * 6;  // 6 indices per side, 6 sides
    config.indices = kallocate(sizeof(u32) * config.index_count, MEMORY_TAG_ARRAY);

    f32 half_width = width * 0.5f;
    f32 half_height = height * 0.5f;
    f32 half_depth = depth * 0.5f;
    f32 min_x = -half_width;
    f32 min_y = -half_height;
    f32 min_z = -half_depth;
    f32 max_x = half_width;
    f32 max_y = half_height;
    f32 max_z = half_depth;
    f32 min_uvx = 0.0f;
    f32 min_uvy = 0.0f;
    f32 max_uvx = tile_x;
    f32 max_uvy = tile_y;

    vertex_3d verts[24];

    // front face
    verts[(0 * 4) + 0].position = (vec3){min_x, min_y, max_z};
    verts[(0 * 4) + 1].position = (vec3){max_x, max_y, max_z};
    verts[(0 * 4) + 2].position = (vec3){min_x, max_y, max_z};
    verts[(0 * 4) + 3].position = (vec3){max_x, min_y, max_z};
    verts[(0 * 4) + 0].texcoord = (vec2){min_uvx, min_uvy};
    verts[(0 * 4) + 1].texcoord = (vec2){max_uvx, max_uvy};
    verts[(0 * 4) + 2].texcoord = (vec2){min_uvx, max_uvy};
    verts[(0 * 4) + 3].texcoord = (vec2){max_uvx, min_uvy};
    verts[(0 * 4) + 0].normal = (vec3){0.0f, 0.0f, 1.0f};
    verts[(0 * 4) + 1].normal = (vec3){0.0f, 0.0f, 1.0f};
    verts[(0 * 4) + 2].normal = (vec3){0.0f, 0.0f, 1.0f};
    verts[(0 * 4) + 3].normal = (vec3){0.0f, 0.0f, 1.0f};

    // Back face
    verts[(1 * 4) + 0].position = (vec3){max_x, min_y, min_z};
    verts[(1 * 4) + 1].position = (vec3){min_x, max_y, min_z};
    verts[(1 * 4) + 2].position = (vec3){max_x, max_y, min_z};
    verts[(1 * 4) + 3].position = (vec3){min_x, min_y, min_z};
    verts[(1 * 4) + 0].texcoord = (vec2){min_uvx, min_uvy};
    verts[(1 * 4) + 1].texcoord = (vec2){max_uvx, max_uvy};
    verts[(1 * 4) + 2].texcoord = (vec2){min_uvx, max_uvy};
    verts[(1 * 4) + 3].texcoord = (vec2){max_uvx, min_uvy};
    verts[(1 * 4) + 0].normal = (vec3){0.0f, 0.0f, -1.0f};
    verts[(1 * 4) + 1].normal = (vec3){0.0f, 0.0f, -1.0f};
    verts[(1 * 4) + 2].normal = (vec3){0.0f, 0.0f, -1.0f};
    verts[(1 * 4) + 3].normal = (vec3){0.0f, 0.0f, -1.0f};

    // left face
    verts[(2 * 4) + 0].position = (vec3){min_x, min_y, min_z};
    verts[(2 * 4) + 1].position = (vec3){min_x, max_y, max_z};
    verts[(2 * 4) + 2].position = (vec3){min_x, max_y, min_z};
    verts[(2 * 4) + 3].position = (vec3){min_x, min_y, max_z};
    verts[(2 * 4) + 0].texcoord = (vec2){min_uvx, min_uvy};
    verts[(2 * 4) + 1].texcoord = (vec2){max_uvx, max_uvy};
    verts[(2 * 4) + 2].texcoord = (vec2){min_uvx, max_uvy};
    verts[(2 * 4) + 3].texcoord = (vec2){max_uvx, min_uvy};
    verts[(2 * 4) + 0].normal = (vec3){-1.0f, 0.0f, 0.0f};
    verts[(2 * 4) + 1].normal = (vec3){-1.0f, 0.0f, 0.0f};
    verts[(2 * 4) + 2].normal = (vec3){-1.0f, 0.0f, 0.0f};
    verts[(2 * 4) + 3].normal = (vec3){-1.0f, 0.0f, 0.0f};

    // right face
    verts[(3 * 4) + 0].position = (vec3){max_x, min_y, max_z};
    verts[(3 * 4) + 1].position = (vec3){max_x, max_y, min_z};
    verts[(3 * 4) + 2].position = (vec3){max_x, max_y, max_z};
    verts[(3 * 4) + 3].position = (vec3){max_x, min_y, min_z};
    verts[(3 * 4) + 0].texcoord = (vec2){min_uvx, min_uvy};
    verts[(3 * 4) + 1].texcoord = (vec2){max_uvx, max_uvy};
    verts[(3 * 4) + 2].texcoord = (vec2){min_uvx, max_uvy};
    verts[(3 * 4) + 3].texcoord = (vec2){max_uvx, min_uvy};
    verts[(3 * 4) + 0].normal = (vec3){1.0f, 0.0f, 0.0f};
    verts[(3 * 4) + 1].normal = (vec3){1.0f, 0.0f, 0.0f};
    verts[(3 * 4) + 2].normal = (vec3){1.0f, 0.0f, 0.0f};
    verts[(3 * 4) + 3].normal = (vec3){1.0f, 0.0f, 0.0f};

    // bottom face
    verts[(4 * 4) + 0].position = (vec3){max_x, min_y, max_z};
    verts[(4 * 4) + 1].position = (vec3){min_x, min_y, min_z};
    verts[(4 * 4) + 2].position = (vec3){max_x, min_y, min_z};
    verts[(4 * 4) + 3].position = (vec3){min_x, min_y, max_z};
    verts[(4 * 4) + 0].texcoord = (vec2){min_uvx, min_uvy};
    verts[(4 * 4) + 1].texcoord = (vec2){max_uvx, max_uvy};
    verts[(4 * 4) + 2].texcoord = (vec2){min_uvx, max_uvy};
    verts[(4 * 4) + 3].texcoord = (vec2){max_uvx, min_uvy};
    verts[(4 * 4) + 0].normal = (vec3){0.0f, -1.0f, 0.0f};
    verts[(4 * 4) + 1].normal = (vec3){0.0f, -1.0f, 0.0f};
    verts[(4 * 4) + 2].normal = (vec3){0.0f, -1.0f, 0.0f};
    verts[(4 * 4) + 3].normal = (vec3){0.0f, -1.0f, 0.0f};

    // top face
    verts[(5 * 4) + 0].position = (vec3){min_x, max_y, max_z};
    verts[(5 * 4) + 1].position = (vec3){max_x, max_y, min_z};
    verts[(5 * 4) + 2].position = (vec3){min_x, max_y, min_z};
    verts[(5 * 4) + 3].position = (vec3){max_x, max_y, max_z};
    verts[(5 * 4) + 0].texcoord = (vec2){min_uvx, min_uvy};
    verts[(5 * 4) + 1].texcoord = (vec2){max_uvx, max_uvy};
    verts[(5 * 4) + 2].texcoord = (vec2){min_uvx, max_uvy};
    verts[(5 * 4) + 3].texcoord = (vec2){max_uvx, min_uvy};
    verts[(5 * 4) + 0].normal = (vec3){0.0f, 1.0f, 0.0f};
    verts[(5 * 4) + 1].normal = (vec3){0.0f, 1.0f, 0.0f};
    verts[(5 * 4) + 2].normal = (vec3){0.0f, 1.0f, 0.0f};
    verts[(5 * 4) + 3].normal = (vec3){0.0f, 1.0f, 0.0f};

    kcopy_memory(config.vertices, verts, config.vertex_size * config.vertex_count);

    for (u32 i = 0; i < 6; ++i) {
        u32 v_offset = i * 4;
        u32 i_offset = i * 6;
        ((u32*)config.indices)[i_offset + 0] = v_offset + 0;
        ((u32*)config.indices)[i_offset + 1] = v_offset + 1;
        ((u32*)config.indices)[i_offset + 2] = v_offset + 2;
        ((u32*)config.indices)[i_offset + 3] = v_offset + 0;
        ((u32*)config.indices)[i_offset + 4] = v_offset + 3;
        ((u32*)config.indices)[i_offset + 5] = v_offset + 1;
    }

    if (name && string_length(name) > 0) {
        string_ncopy(config.name, name, GEOMETRY_NAME_MAX_LENGTH);
    } else {
        string_ncopy(config.name, DEFAULT_GEOMETRY_NAME, GEOMETRY_NAME_MAX_LENGTH);
    }

    if (material_name && string_length(material_name) > 0) {
        string_ncopy(config.material_name, material_name, MATERIAL_NAME_MAX_LENGTH);
    } else {
        string_ncopy(config.material_name, DEFAULT_MATERIAL_NAME, MATERIAL_NAME_MAX_LENGTH);
    }

    return config;
}