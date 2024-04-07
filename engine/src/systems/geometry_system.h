#pragma once

#include "renderer/renderer_types.inl"

typedef struct geometry_system_config {
    // max number of geometries the can be loaded at once.
    // NOTE: Should be significantly greater than the number of static meshes because there can and will be more than one of these per mesh
    // take other systems into account as well
    u32 max_geometry_count;
} geometry_system_config;

// where we are going to store the configurations for the geometry - for each piece of geometry
typedef struct geometry_config {
    u32 vertex_count;     // number of vertices in the geometry
    vertex_3d* vertices;  // pointer to the array of vertices for the geometry
    u32 index_count;      // number of indices in the geometry
    u32* indices;         // pointer to the array of indices for the geometry
    char name[GEOMETRY_NAME_MAX_LENGTH];
    char material_name[MATERIAL_NAME_MAX_LENGTH];
} geometry_config;

#define DEFAULT_GEOMETRY_NAME "default"

// initialize the geometry system
b8 geometry_system_initialize(u64* memory_requirement, void* state, geometry_system_config config);

// shutdown the geometry system
void geometry_system_shutdown(void* state);

// @brief Aquires and existing geometry by id
// @param id the geometry identifier to acquire by
// @return a pointer to the acquired geometry or nullptr if failed
geometry* geometry_system_acquire_by_id(u32 id);

// @brief registers and acquires a new geometry using the given config
// @param config the geometry configuration
// @param auto_release Indicates if the aquired geometry should be unloaded when its refence count reaches 0
// @return a pointer to the acquired geometry or nullptr if failed
geometry* geometry_system_acquire_from_config(geometry_config config, b8 auto_release);

// @brief realeases a reference to the provided geometry
// @param geometry the geometry to be released
void geometry_system_release(geometry* geometry);

// @brief obtains a pointer to the default geometry
// @return a pointer to the default geometry
geometry* geometry_system_get_default();

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
geometry_config geometry_system_generate_plane_config(f32 width, f32 height, u32 x_segment_count, u32 y_segment_count, f32 tile_x, f32 tile_y, const char* name, const char* material_name);
