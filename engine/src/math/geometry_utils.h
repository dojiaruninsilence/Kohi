#pragma once

#include "math_types.h"

// @brief calculates normals for the given vertex and index data. modifies vertices in place
// @param vertex_count the number of vertices
// @param vertices an array of vertices
// @param index_count the number of indices.
// @param indices an array of indices
void geometry_generate_normals(u32 vertex_count, vertex_3d* vertices, u32 index_count, u32* indices);

// @brief calculates tangents for the given vertex and index data. modifies vertices in place
// @param vertex_count the number of vertices
// @param vertices an array of vertices
// @param index_count the number of indices.
// @param indices an array of indices
void geometry_generate_tangents(u32 vertex_count, vertex_3d* vertices, u32 index_count, u32* indices);