// bogus error i believe
#include "application.h"
#include "game_types.h"

#include "logger.h"

#include "platform/platform.h"
#include "core/kmemory.h"
#include "core/event.h"
#include "core/input.h"
#include "core/clock.h"

#include "renderer/renderer_frontend.h"

// there will only be one instance of application running

typedef struct application_state {
    game* game_inst;
    b8 is_running;
    b8 is_suspended;
    platform_state platform;  // pointer to platform state - dynamic allocation? need to look all these up
    i16 width;
    i16 height;
    clock clock;
    f64 last_time;  // has to do with the game loop
} application_state;

static b8 initialized = FALSE;  // prevent calling application create more than once. which would cause a failure
static application_state app_state;

// foreward declared functions, look up what this means - private functions
// event handlers
b8 application_on_event(u16 code, void* sender, void* listener_inst, event_context context);   // genaric -- pass in the code, a pointer to the sender, a pointer to the instance, and the context
b8 application_on_key(u16 code, void* sender, void* listener_inst, event_context context);     // on key -- pass in the code, a pointer to the sender, a pointer to the instance, and the context
b8 application_on_resized(u16 code, void* sender, void* listener_inst, event_context context);  // on resized event, pass in the code, a pointer to the sender, a pointer to the instance, and the context

b8 application_create(game* game_inst) {                      // this error does not seem to actually be an error
    if (initialized) {                                        // initialized is set to true
        KERROR("application_create called more than once.");  // throw an error
        return FALSE;                                         // boot out
    }

    app_state.game_inst = game_inst;  // set game instance. from game_types.h

    // initialize subsystems for the application here
    initilize_logging();  // logger.h in core foleder
    input_initialize();   // initialize the input subsystem

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
    event_register(EVENT_CODE_RESIZED, 0, application_on_resized);

    // start the platform layer and input the config data from the application_config struct
    if (!platform_startup(&app_state.platform,
                          game_inst->app_config.name,
                          game_inst->app_config.start_pos_x,
                          game_inst->app_config.start_pos_y,
                          game_inst->app_config.start_width,
                          game_inst->app_config.start_height)) {
        return FALSE;
    }

    // renderer startup
    if (!renderer_initialize(game_inst->app_config.name, &app_state.platform)) {
        KFATAL("Game failed to initialize.");
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
    // start the clock for the app
    clock_start(&app_state.clock);                  // start the clock for the app state
    clock_update(&app_state.clock);                 // update the clock for the app state - update the elapsed time
    app_state.last_time = app_state.clock.elapsed;  // set last time to the elapsed time
    f64 running_time = 0;                           // declare with 0 - to keep track of how much time has accumulated
    u8 frame_count = 0;                             // declare with 0 - to keep track of the frames per second
    f64 target_frame_seconds = 1.0f / 60;           // target frame rate of 60 frames per second - so this gives us a 60th of a second 1/60s - for places where the frame rate may need to be limited

    // test of the memory subsystem
    KINFO(get_memory_usage_str())
    // this is basically the "game" loop at the moment will run as long as app state remains true
    while (app_state.is_running) {
        if (!platform_pump_messages(&app_state.platform)) {  // if there are no events return false and shut the app doen
            app_state.is_running = FALSE;                    // shut down application layer
        }

        if (!app_state.is_suspended) {
            // update clock and get delta time
            clock_update(&app_state.clock);                       // update the elapsed time
            f64 current_time = app_state.clock.elapsed;           // grab the clocks current elapsed time
            f64 delta = (current_time - app_state.last_time);     // create delta by taking the current time and subtracting from it the last time
            f64 frame_start_time = platform_get_absolute_time();  // get the time from the os and set it to frame start time - to keep track of how long each frame takes to render

            if (!app_state.game_inst->update(app_state.game_inst, (f32)delta)) {  // run the update routine. the zero is in polace of delta time for now, will be fixed later
                KFATAL("Game update failed, shutting down.");
                app_state.is_running = FALSE;  // shut down the application layer
                break;
            }

            // call the games render routine
            if (!app_state.game_inst->render(app_state.game_inst, (f32)delta)) {  // again the zero is in place of delta time
                KFATAL("Game renderer failed, shutting down");
                app_state.is_running = FALSE;
                break;
            }

            // this is not how this will be done in the future
            // TODO: refactor packet creation
            render_packet packet;
            packet.delta_time = delta;
            renderer_draw_frame(&packet);  // here is where the draw calls are going to be?

            // figure out how long the frame took and, if below
            f64 frame_end_time = platform_get_absolute_time();                  // get time from os and set to frame end time
            f64 frame_elapsed_time = frame_end_time - frame_start_time;         // get the elapsed time from the start and end times
            running_time += frame_elapsed_time;                                 // increment the running time by the frame elapsed time
            f64 remaining_seconds = target_frame_seconds - frame_elapsed_time;  // take the elapsed time away from one second

            if (remaining_seconds > 0) {                        // if there are still ms left
                u64 remaining_ms = (remaining_seconds * 1000);  // conver to ms

                // if there is time left, give it back to the os - helps with performance
                b8 limit_frames = FALSE;  // a switch for enabling the below statement
                if (remaining_ms > 0 && limit_frames) {
                    platform_sleep(remaining_ms - 1);
                }

                frame_count++;  // increment the frame count
            }

            // NOTE: input update/state copying should always be handled after any input should be recorder, i.e. before this line
            // as a safety, input is the last thing to be updated before this frame ends
            input_update(delta);

            // update last time
            app_state.last_time = current_time;  // at the very end set last time to the current time
        }
    }

    app_state.is_running = FALSE;  // in the event it exits the loop while true make sure it shutsdown

    // event listeners - unregister
    event_unregister(EVENT_CODE_APPLICATION_QUIT, 0, application_on_event);
    event_unregister(EVENT_CODE_KEY_PRESSED, 0, application_on_key);
    event_unregister(EVENT_CODE_KEY_RELEASED, 0, application_on_key);    
    event_unregister(EVENT_CODE_RESIZED, 0, application_on_resized);

    event_shutdown();  // shutdown the event system
    input_shutdown();  // shutdown the input system

    renderer_shutdown();  // shutdown the renderer

    platform_shutdown(&app_state.platform);  // shut down the platform layer

    return TRUE;
}

// a way to pass the window size to the renderer, passes out a width and height
void application_get_framebuffer_size(u32* width, u32* height) {
    *width = app_state.width;    // pass the app state width to width
    *height = app_state.height;  // pass the app state height to height
}

b8 application_on_event(u16 code, void* sender, void* listener_inst, event_context context) {
    switch (code) {                          // chaeck to see what code was passed
        case EVENT_CODE_APPLICATION_QUIT: {  // if code is app quit
            KINFO("EVENT_CODE_APPLICATION_QUIT recieved, shutting down.\n");
            app_state.is_running = FALSE;  // switch off the app
            return TRUE;                   // blocks the app quit from going anywhere else
        }
    }

    return FALSE;  // if the code not in the list
}

b8 application_on_key(u16 code, void* sender, void* listener_inst, event_context context) {
    if (code == EVENT_CODE_KEY_PRESSED) {    // was it a key press
        u16 key_code = context.data.u16[0];  // set key code to the key data in context
        if (key_code == KEY_ESCAPE) {        // if the key is esc
            // NOTE: technically firing an event to itself, but eventually there may be other listeners
            event_context data = {};                           // create empty context, data
            event_fire(EVENT_CODE_APPLICATION_QUIT, 0, data);  // fire the event

            // block anything else from processing this
            return TRUE;
        } else if (key_code == KEY_A) {
            // example on checking for a key
            KDEBUG("Explicit - A key pressed!");
        } else {
            KDEBUG("'%c' key pressed in window.", key_code);  // if a wasnt pressed, what was pressed
        }
    } else if (code == EVENT_CODE_KEY_RELEASED) {
        u16 key_code = context.data.u16[0];
        if (key_code == KEY_B) {
            // example for checking for a key
            KDEBUG("Explicit - B key released!");
        } else {
            KDEBUG("'%c' key released in window.", key_code);  // if b wasnt released, what was
        }
    }
    return FALSE;
}

// on resized event, pass in the code, a pointer to the sender, a pointer to the instance, and the context - this is an event handler
b8 application_on_resized(u16 code, void* sender, void* listener_inst, event_context context) {
    if (code == EVENT_CODE_RESIZED) {      // if it is a resized event
        u16 width = context.data.u16[0];   // from the u16 array in context data get the width from index 0
        u16 height = context.data.u16[1];  // and the height from index 1

        // check if different. if so, trigger a resize event
        if (width != app_state.width || height != app_state.height) {
            app_state.width = width;    // pass in the width from the context
            app_state.height = height;  // pass in the height from the context

            KDEBUG("Window resize: %i, %i", width, height);  // display the dimendions in a debug msg

            // handle minimization
            if (width == 0 || height == 0) {                                // if either the width or the height are 0
                KINFO("Window is minimized, suspending the application.");  // throw a message
                app_state.is_suspended = TRUE;                              // set the app state to suspended
                return TRUE;
            } else {                                                  // if width and height arent 0
                if (app_state.is_suspended) {                         // if app state is suspended
                    KINFO("Window restored, resuming application.");  // throw an info msg
                    app_state.is_suspended = FALSE;                   // ans set the app state to not suspended
                }
                app_state.game_inst->on_resize(app_state.game_inst, width, height);  // call the function pointer on resize, pass in  the game inst, and the dimaensions
                renderer_on_resized(width, height);                                  // call the renderer on resized function, and pass through the width and height
            }
        }
    }

    // event purposefully not handled to allow other listeners to get this
    return FALSE;
}
