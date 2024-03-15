#pragma once

#include "core/application.h"
#include "core/logger.h"
#include "core/kmemory.h"
#include "game_types.h"

// Externally-Defined function to create a game.
extern b8 create_game(game* out_game);  // tell compiler we have an externally defined function to create a game, tells it not to worry about where it is coming from -- gonna replace main in testbed

// this is now the main entry point for the entire application
int main(void) {
    initialize_memory();

    // request the game instance from the application
    game game_inst;
    if (!create_game(&game_inst)) {
        KFATAL("Coud not create Game!");
        return -1;
    }

    // ensure that the function pointers exsist
    if (!game_inst.render || !game_inst.update || !game_inst.initialize || !game_inst.on_resize) {
        KFATAL("The game's function pointers must be assigned!");
        return -2;
    }

    // create the game? - initialization
    if (!application_create(&game_inst)) {  // passes the address
        KFATAL("Application failed to create!");
        return 1;
    }

    // begin the game loop
    if (!application_run()) {
        KINFO("Application did not shutsown gracefully.");
        return 2;
    }

    shutdown_memory();

    return 0;
}