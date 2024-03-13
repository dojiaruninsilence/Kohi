#include "game.h"

#include <entry.h>

// TODO: remove
#include <platform/platform.h>

// define the function to create a game
b8 create_game(game* out_game) {
    // application configuration
    out_game->app_config.start_pos_x = 100;
    out_game->app_config.start_pos_y = 100;
    out_game->app_config.start_width = 1280;
    out_game->app_config.start_height = 720;
    out_game->app_config.name = "Kohi Engine Testbed";
    out_game->update = game_update;          // send these functions to the engine
    out_game->render = game_render;          // send these functions to the engine
    out_game->initialize = game_initialize;  // send these functions to the engine
    out_game->on_resize = game_on_resize;    // send these functions to the engine

    // create the game state
    out_game->state = platform_allocate(sizeof(game_state), FALSE);

    return TRUE;
}