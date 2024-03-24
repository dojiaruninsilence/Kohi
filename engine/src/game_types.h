#pragma once

#include "core/application.h"

// this will represent the basic game state in a game.
// called for creation by the application

typedef struct game {
    // the application configuration
    application_config app_config;

    // function pointer to game's initialize function. -- tell the game to do whatever it needs to initialize
    b8 (*initialize)(struct game* game_inst);

    // function pointer to game's update function
    b8 (*update)(struct game* game_inst, f32 delta_time);

    // function pointer to game's render function
    b8 (*render)(struct game* game_inst, f32 delta_time);

    // function pointer to handle resizes, if applicable
    void (*on_resize)(struct game* game_inst, u32 width, u32 height);

    // game specific game state. created and managed by the game
    void* state;

    // store a pointer to the application state -- makes sense to allocate all of this at the same time - pointer cause we dont want the game to actually have access to the application
    void* application_state;
    ;
} game;