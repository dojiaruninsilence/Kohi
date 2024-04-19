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

// @brief de-duplicates vertices, leaving only unique ones. leaves the original vertices array intact. allocates a new array
// in out_vertices. modifies indices in place. original vertex array should be freed by the caller
// @param vertex_count the number of vertices in the array
// @param vertices the original array of vertices to be de - duplicated. not modified.
// @param index_count the number of indices in the array
// @param indices the array of indices. modified in place as vertices are removed
// @param out_vertex_count a pointer to hold the final vertex count
// @param out_vertices a pointer to hold the array of de-duplicated vertices
void geometry_deduplicate_vertices(u32 vertex_count, vertex_3d* vertices, u32 index_count, u32* indices, u32* out_vertex_count, vertex_3d** out_vertices);