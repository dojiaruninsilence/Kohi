// bogus error i believe
#include "application.h"
#include "game_types.h"

#include "logger.h"

#include "platform/platform.h"
#include "core/kmemory.h"

// there will only be one instance of application running

typedef struct application_state {
    game* game_inst;
    b8 is_running;
    b8 is_suspended;
    platform_state platform;  // pointer to platform state - dynamic allocation? need to look all these up
    i16 width;
    i16 height;
    f64 last_time;  // has to do with the game loop
} application_state;

static b8 initialized = FALSE;  // prevent calling application create more than once. which would cause a failure
static application_state app_state;

b8 application_create(game* game_inst) {  // this error does not seem to actually be an error
    if (initialized) {
        KERROR("application_create called more than once.");
        return FALSE;
    }

    app_state.game_inst = game_inst;  // set game instance. from game_types.h

    // initialize subsystems for the application here
    initilize_logging();  // logger.h in core foleder

    // test stuff TODO: will be removed
    KFATAL("a test message: %f", 3.14f);
    KERROR("a test message: %f", 3.14f);
    KWARN("a test message: %f", 3.14f);
    KINFO("a test message: %f", 3.14f);
    KDEBUG("a test message: %f", 3.14f);
    KTRACE("a test message: %f", 3.14f);

    app_state.is_running = TRUE;     // boolean to say the app is running
    app_state.is_suspended = FALSE;  // suspended is a state in which the application shouldnt be updating or anything - will implement later

    // start the platform layer and input the config data from the application_config struct
    if (!platform_startup(&app_state.platform,
                        game_inst->app_config.name,
                        game_inst->app_config.start_pos_x,
                        game_inst->app_config.start_pos_y,
                        game_inst->app_config.start_width,
                        game_inst->app_config.start_height)) {
        return FALSE;
    }

    // initialize the game
    if (!app_state.game_inst->initialize(app_state.game_inst)) {
        KFATAL("Game failed to initialize.")
        return FALSE;
    }

    // attach event handler for resizing events -- not ready for this to work yet but getting it ready
    app_state.game_inst->on_resize(app_state.game_inst, app_state.width, app_state.height);  // game gets the window dimensions when they change - set the app state to fit the dimensions when changed

    initialized = TRUE;

    return TRUE;
}

b8 application_run() {
    // test of the memory subsystem
    KINFO(get_memory_usage_str())
    // this is basically the "game" loop at the moment will run as long as app state remains true
    while (app_state.is_running) {
        if (!platform_pump_messages(&app_state.platform)) {  // if there are no events return false and shut the app doen
            app_state.is_running = FALSE;                    // shut down application layer
        }

        if (!app_state.is_suspended) {
            if (!app_state.game_inst->update(app_state.game_inst, (f32)0)) {  // run the update routine. the zero is in polace of delta time for now, will be fixed later
                KFATAL("Game update failed, shutting down.");
                app_state.is_running = FALSE;  // shut down the application layer
                break;
            }

            // call the games render routine
            if (!app_state.game_inst->render(app_state.game_inst, (f32)0)) {  // again the zero is in place of delta time
                KFATAL("Game renderer failed, shutting down");
                app_state.is_running = FALSE;
                break;
            }
        }
    }

    app_state.is_running = FALSE;  // in the event it exits the loop while true make sure it shutsdown

    platform_shutdown(&app_state.platform);  // shut doen the platform layer

    return TRUE;
}
