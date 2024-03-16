#pragma once

#include "renderer_types.inl"

// foreward declarations
struct static_mesh_data;
struct platform_state;

// every subsystem needs these
b8 renderer_initialize(const char* application_name, struct platform_state* plat_state);  // takes in pointers to the application name and the platform state
void renderer_shutdown();

void renderer_on_resized(u16 width, u16 height);  // may change the structure of this later, keeping it simple for now

// called once per frame to take off all the rendering
// a renderer packet is a packet full of information prepared ahead of time
// contains all the info that the renderer needs to know to draw that frame
b8 renderer_draw_frame(render_packet* packet);
