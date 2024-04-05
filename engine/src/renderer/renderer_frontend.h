#pragma once

#include "renderer_types.inl"

// initialize the renderer subsystem, - always call twice - on first pass pass in the memory requirement to get the memory required, and zero for the state
// on the second pass - pass in the state as well as the memory rewuirement and actually initialize the subsystem, also pass in a pointer to the application name
b8 renderer_system_initialize(u64* memory_requirement, void* state, const char* application_name);  // takes in pointers to the application name and the platform state
void renderer_system_shutdown(void* state);

void renderer_on_resized(u16 width, u16 height);  // may change the structure of this later, keeping it simple for now

// called once per frame to take off all the rendering
// a renderer packet is a packet full of information prepared ahead of time
// contains all the info that the renderer needs to know to draw that frame
b8 renderer_draw_frame(render_packet* packet);

// HACK: this should not be exposed outside the engine
KAPI void renderer_set_view(mat4 view);

// create a texture pass in a pointer to the pixels in a u8 array, that is 8 bits per pixel and an address for the texture struct
void renderer_create_texture(const u8* pixels, struct texture* texture);

// destroy a texture
void renderer_destroy_texture(struct texture* texture);

b8 renderer_create_material(struct material* material);
void renderer_destroy_material(struct material* material);