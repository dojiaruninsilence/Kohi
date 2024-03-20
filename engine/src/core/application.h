#pragma once

#include "defines.h"

struct game;

// pay no mind to the bogus warning here
// application configuration - to pass in any of the variables me want to use
typedef struct application_config {
    // window starting position x axis, if applicable
    i16 start_pos_x;

    // window starting position y axis, if applicable
    i16 start_pos_y;

    // window starting width, if applicable
    i16 start_width;

    // window starting height, if applicable
    i16 start_height;

    // the application name used in the window, if applicable
    char* name;
} application_config;

// self explanitory, use in external applications to keep user code and engine code separated
KAPI b8 application_create(struct game* game_inst);

KAPI b8 application_run();

void application_get_framebuffer_size(u32* width, u32* height);  // a way to pass the window size to the renderer, passes out a width and height
