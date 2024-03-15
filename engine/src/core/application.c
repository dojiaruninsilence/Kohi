// bogus error i believe
#include "application.h"
#include "game_types.h"

#include "logger.h"

#include "platform/platform.h"
#include "core/kmemory.h"
#include "core/event.h"
#include "core/input.h" 

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

// foreward declared functions, look up what this means
// event handlers
b8 application_on_event(u16 code, void* sender, void* listener_inst, event_context context); // genaric
b8 application_on_key(u16 code, void* sender, void* listener_inst, event_context context); // on key

b8 application_create(game* game_inst) {  // this error does not seem to actually be an error
    if (initialized) {
        KERROR("application_create called more than once.");
        return FALSE;
    }

    app_state.game_inst = game_inst;  // set game instance. from game_types.h

    // initialize subsystems for the application here
    initilize_logging();  // logger.h in core foleder
    input_initialize();  // initialize the input subsystem

    // test stuff TODO: will be removed
    KFATAL("a test message: %f", 3.14f);
    KERROR("a test message: %f", 3.14f);
    KWARN("a test message: %f", 3.14f);
    KINFO("a test message: %f", 3.14f);
    KDEBUG("a test message: %f", 3.14f);
    KTRACE("a test message: %f", 3.14f);

    app_state.is_running = TRUE;     // boolean to say the app is running
    app_state.is_suspended = FALSE;  // suspended is a state in which the application shouldnt be updating or anything - will implement later

    // initialize events and verify it worked
    if (!event_initialize()) {
        KERROR("Event System failed initialization. Application cannot continue");
        return FALSE;
    }

    // event listeners - register
    event_register(EVENT_CODE_APPLICATION_QUIT, 0, application_on_event);
    event_register(EVENT_CODE_KEY_PRESSED, 0, application_on_key);
    event_register(EVENT_CODE_KEY_RELEASED, 0, application_on_key);

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
            // NOTE: input update/state copying should always be handled after any input should be recorder, i.e. before this line
            // as a safety, input is the last thing to be updated before this frame ends
            input_update(0);
        }
    }

    app_state.is_running = FALSE;  // in the event it exits the loop while true make sure it shutsdown

    // event listeners - unregister
    event_unregister(EVENT_CODE_APPLICATION_QUIT, 0, application_on_event);
    event_unregister(EVENT_CODE_KEY_PRESSED, 0, application_on_key);
    event_unregister(EVENT_CODE_KEY_RELEASED, 0, application_on_key);

    event_shutdown();  // shutdown the event system
    input_shutdown();  // shutdown the input system

    platform_shutdown(&app_state.platform);  // shut down the platform layer

    return TRUE;
}

b8 application_on_event(u16 code, void* sender, void* listener_inst, event_context context) {
    switch (code) { // chaeck to see what code was passed
        case EVENT_CODE_APPLICATION_QUIT: { // if code is app quit
            KINFO("EVENT_CODE_APPLICATION_QUIT recieved, shutting down.\n");
            app_state.is_running = FALSE; // switch off the app
            return TRUE; // blocks the app quit from going anywhere else
        } 
    }

    return FALSE; // if the code not in the list
}

b8 application_on_key(u16 code, void* sender, void* listener_inst, event_context context){
    if (code == EVENT_CODE_KEY_PRESSED) { // was it a key press
        u16 key_code = context.data.u16[0]; // set key code to the key data in context
        if (key_code == KEY_ESCAPE) { // if the key is esc
            // NOTE: technically firing an event to itself, but eventually there may be other listeners
            event_context data = {}; // create empty context, data
            event_fire(EVENT_CODE_APPLICATION_QUIT, 0, data); // fire the event

            // block anything else from processing this
            return TRUE;
        } else if (key_code == KEY_A) {
            // example on checking for a key
            KDEBUG("Explicit - A key pressed!");
        } else {
            KDEBUG("'%c' key pressed in window.", key_code); // if a wasnt pressed, what was pressed
        }
    } else if (code == EVENT_CODE_KEY_RELEASED) {
        u16 key_code = context.data.u16[0];
        if (key_code == KEY_B) {
            // example for checking for a key
            KDEBUG("Explicit - B key released!");
        } else {
            KDEBUG("'%c' key released in window.", key_code); // if b wasnt released, what was
        }
    }
    return FALSE;
}
