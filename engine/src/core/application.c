// bogus error i believe
#include "application.h"
#include "game_types.h"

#include "logger.h"

#include "platform/platform.h"
#include "core/kmemory.h"
#include "core/event.h"
#include "core/input.h"
#include "core/clock.h"
#include "core/kstring.h"

#include "memory/linear_allocator.h"

#include "renderer/renderer_frontend.h"

// systems
#include "systems/texture_system.h"
#include "systems/material_system.h"
#include "systems/geometry_system.h"
#include "systems/resource_system.h"
#include "systems/shader_system.h"

// TODO: temp
#include "math/kmath.h"
// TODO: temp

// there will only be one instance of application running

typedef struct application_state {
    game* game_inst;
    b8 is_running;
    b8 is_suspended;
    i16 width;
    i16 height;
    clock clock;
    f64 last_time;  // has to do with the game loop

    // for systems states memory allocation
    linear_allocator systems_allocator;  // where all the info for sytem states is going to be stored

    // event system state allocation
    u64 event_system_memory_requirement;  // where the amount of storage that is needed for the event system is stored
    void* event_system_state;             // a pointer to where the event state is being store

    // logger system state allocation
    u64 logging_system_memory_requirement;  // where the amount of storage that is needed for the logger system is stored
    void* logging_system_state;             // a pointer to where the logger system state is being stored

    // input system state allocation
    u64 input_system_memory_requirement;  // where the amount of storage that is needed for the input system is stored
    void* input_system_state;             // a pointer to where the input state is being store

    // platform system state allocation
    u64 platform_system_memory_requirement;  // where the amount of storage that is needed for the platform system is stored
    void* platform_system_state;             // a pointer to where the platform state is being store

    // resource system state allocation
    u64 resource_system_memory_requirement;  // where the amount of storage that is needed for the resource system is stored
    void* resource_system_state;             // a pointer to where the resource state is being store

    u64 shader_system_memory_requirement;
    void* shader_system_state;

    // renderer system state allocation
    u64 renderer_system_memory_requirement;  // where the amount of storage that is needed for the renderer system is stored
    void* renderer_system_state;             // a pointer to where the renderer state is being store

    // texture system state allocation
    u64 texture_system_memory_requirement;  // where the amount of storage that is needed fot the texture system is stored
    void* texture_system_state;             // a pointer to where the texture system state is being stored

    // material system state allocation
    u64 material_system_memory_requirement;  // where the amount of storage that is needed fot the material system is stored
    void* material_system_state;             // a pointer to where the material system state is being stored

    // geometry system state allocation
    u64 geometry_system_memory_requirement;  // where the amount of storage that is needed fot the geometry system is stored
    void* geometry_system_state;             // a pointer to where the geometry system state is being stored

    // TODO: temp
    geometry* test_geometry;
    geometry* test_ui_geometry;
    // TODO: end temp

} application_state;

static application_state* app_state;  // define a pointer to the application state -- all we are storing on the stack for now is a pointer to the allocation

// foreward declared functions, look up what this means - private functions
// event handlers
b8 application_on_event(u16 code, void* sender, void* listener_inst, event_context context);    // genaric -- pass in the code, a pointer to the sender, a pointer to the instance, and the context
b8 application_on_key(u16 code, void* sender, void* listener_inst, event_context context);      // on key -- pass in the code, a pointer to the sender, a pointer to the instance, and the context
b8 application_on_resized(u16 code, void* sender, void* listener_inst, event_context context);  // on resized event, pass in the code, a pointer to the sender, a pointer to the instance, and the context

// TODO: temporary
b8 event_on_debug_event(u16 code, void* sender, void* listener_inst, event_context data) {
    const char* names[3] = {
        "cobblestone",
        "paving",
        "paving2"};
    static i8 choice = 2;

    // save off the old name
    const char* old_name = names[choice];

    choice++;     // increment
    choice %= 3;  // then mod back to 0. still need to learn this

    // load up the new texture
    if (app_state->test_geometry) {
        app_state->test_geometry->material->diffuse_map.texture = texture_system_aquire(names[choice], true);
        if (!app_state->test_geometry->material->diffuse_map.texture) {
            KWARN("event_on_debug_event no texture! using default");
            app_state->test_geometry->material->diffuse_map.texture = texture_system_get_default_texture();
        }

        // release the old texture
        texture_system_release(old_name);
    }

    return true;
}
// TODO: end temporary

// create a game, this will include evey external thing, like testbed or an editor
b8 application_create(game* game_inst) {
    if (game_inst->application_state) {                       // can only run one application state, so check if one is initialized already
        KERROR("application_create called more than once.");  // throw an error
        return false;                                         // boot out
    }

    // memory sytem must be the first thing to be stood up
    memory_system_configuration memory_system_config = {};
    memory_system_config.total_alloc_size = GIBIBYTES(1);
    if (!memory_system_initialize(memory_system_config)) {
        KERROR("Failed to initialize memory system; shutting down.");
        return false;
    }

    // allocate the game state
    game_inst->state = kallocate(game_inst->state_memory_requirement, MEMORY_TAG_GAME);

    // allocate memory for the application state, this will eventually move, and for now will be one of the few dynamica allocations in the code base
    game_inst->application_state = kallocate(sizeof(application_state), MEMORY_TAG_APPLICATION);  // use the size of the application state and tag it with application
    app_state = game_inst->application_state;                                                     // set the app state pointer to the game instance application state
    app_state->game_inst = game_inst;                                                             // set game instance. from game_types.h
    app_state->is_running = false;                                                                // boolean to say the app is running
    app_state->is_suspended = false;                                                              // suspended is a state in which the application shouldnt be updating or anything - will implement later

    // setup the memory allocation to hold the states of the systems except for the memory system
    // set the size for the lineare allocator for the systems' states
    u64 system_allocator_total_size = 64 * 1024 * 1024;                                      // this is 64mb
    linear_allocator_create(system_allocator_total_size, 0, &app_state->systems_allocator);  // create the linear allocator, give it the address to the app states system allocator, it will allocate its own memory, and the total size

    // initialize other subsystems for the application here

    // initialize the event subsystem
    //  on the first pass, pass in the pointer to the requirement field to get the size required
    event_system_initialize(&app_state->event_system_memory_requirement, 0);
    // allocate memory from the linear allocator for the state of the events system, and give a pointer to that memory to event system state
    app_state->event_system_state = linear_allocator_allocate(&app_state->systems_allocator, app_state->event_system_memory_requirement);
    // second pass actually initializes the event system, pass it a pointer to the required memory, and a pointer to where the memory is
    event_system_initialize(&app_state->event_system_memory_requirement, app_state->event_system_state);

    // initialize the logging system
    initilize_logging(&app_state->logging_system_memory_requirement, 0);  // get the memory required to store the state, pass in the requirement field and 0 so it only gets memory requirement
    // allocate memory from the linear allocator for the state of logger system, and give the pointer to the memory to logging system state
    app_state->logging_system_state = linear_allocator_allocate(&app_state->systems_allocator, app_state->logging_system_memory_requirement);
    if (!initilize_logging(&app_state->logging_system_memory_requirement, app_state->logging_system_state)) {  // actually initialize the logging system , passing in the required memory and the poiter to where the state can be stored - if it fails
        KERROR("Failed to initialize the logging system shutting down.");                                      // throw an error
        return false;                                                                                          // and boot out
    }

    // initialize the input system
    // first pass pass in the the pointer to the requirement field to get the size required
    input_system_initialize(&app_state->input_system_memory_requirement, 0);
    // allocate memory from the linear allocator for the state of input system, and give the pointer to the memory to input system state
    app_state->input_system_state = linear_allocator_allocate(&app_state->systems_allocator, app_state->input_system_memory_requirement);
    // second pass actually initializes the input system, pass it a pointer to the required memory, and a pointer to where the memory is
    input_system_initialize(&app_state->input_system_memory_requirement, app_state->input_system_state);

    // event listeners - register for engine level events
    event_register(EVENT_CODE_APPLICATION_QUIT, 0, application_on_event);
    event_register(EVENT_CODE_KEY_PRESSED, 0, application_on_key);
    event_register(EVENT_CODE_KEY_RELEASED, 0, application_on_key);
    event_register(EVENT_CODE_RESIZED, 0, application_on_resized);
    // TODO: temp
    event_register(EVENT_CODE_DEBUG0, 0, event_on_debug_event);
    // TODO: temp

    // initialize the platform subsystem and layer
    // on the first pass just pass in the pointer to the platform memory requirement field, leave the rest 0s
    platform_system_startup(&app_state->platform_system_memory_requirement, 0, 0, 0, 0, 0, 0);
    // allocate memory from the linear allocator for the state of platform system, and give the pointer to the memory to platform system state
    app_state->platform_system_state = linear_allocator_allocate(&app_state->systems_allocator, app_state->platform_system_memory_requirement);
    if (!platform_system_startup(                            // second pass actually initializes the platfor subsystem
            &app_state->platform_system_memory_requirement,  // pass in the memory requirements
            app_state->platform_system_state,                // a pointer the memory allocated
            game_inst->app_config.name,                      // the name of the app(window name, ect)
            game_inst->app_config.start_pos_x,               // the x pos
            game_inst->app_config.start_pos_y,               // the y pos
            game_inst->app_config.start_width,               // width
            game_inst->app_config.start_height)) {           // and height
        return false;
    }

    // resource system. on the first call only the memory requirements will be returned, memory is then allocated for the resource system
    // on the second call the system is actually initialized
    resource_system_config resource_sys_config;
    resource_sys_config.asset_base_path = "../assets";
    resource_sys_config.max_loader_count = 32;
    resource_system_initialize(&app_state->resource_system_memory_requirement, 0, resource_sys_config);
    app_state->resource_system_state = linear_allocator_allocate(&app_state->systems_allocator, app_state->resource_system_memory_requirement);
    if (!resource_system_initialize(&app_state->resource_system_memory_requirement, app_state->resource_system_state, resource_sys_config)) {
        KFATAL("Failed to initialize resource system. Aborting application.");
        return false;
    }

    // shader system
    shader_system_config shader_sys_config;
    shader_sys_config.max_shader_count = 1024;
    shader_sys_config.max_uniform_count = 128;
    shader_sys_config.max_global_textures = 31;
    shader_sys_config.max_instance_textures = 31;
    shader_system_initialize(&app_state->shader_system_memory_requirement, 0, shader_sys_config);
    app_state->shader_system_state = linear_allocator_allocate(&app_state->systems_allocator, app_state->shader_system_memory_requirement);
    if (!shader_system_initialize(&app_state->shader_system_memory_requirement, app_state->shader_system_state, shader_sys_config)) {
        KFATAL("Failed to initialize shader system. Aborting application.");
        return false;
    }

    //  initialize the renderer subsystem
    // on the first pass just pass in the pointer to the renderer system memory requirement field, leave the rest 0s
    renderer_system_initialize(&app_state->renderer_system_memory_requirement, 0, 0);
    // allocate memory from the linear allocator for the state of renderer system, and give the pointer to the memory to renderer system state
    app_state->renderer_system_state = linear_allocator_allocate(&app_state->systems_allocator, app_state->renderer_system_memory_requirement);
    // second pass actually initializes the input system, pass it a pointer to the required memory, and a pointer to where the memory is, and a pointer to the application name
    if (!renderer_system_initialize(&app_state->renderer_system_memory_requirement, app_state->renderer_system_state, game_inst->app_config.name)) {  // if it fails
        KFATAL("Failed to initialize renderer. Aborting application.");                                                                               // throw a fatal error
        return false;                                                                                                                                 // and boot out
    }

    // texture system
    texture_system_config texture_sys_config;      // define the texture system configuration struct
    texture_sys_config.max_texture_count = 65536;  // max number of textures that can be loaded
    // on the first pass just pass in the pointer to the texture system memory requirement field, and the configuration struct, leave the rest 0
    texture_system_initialize(&app_state->texture_system_memory_requirement, 0, texture_sys_config);
    // allocate memory from the linear allocator for the state of texture system, and give the pointer to the memory to texture system state
    app_state->texture_system_state = linear_allocator_allocate(&app_state->systems_allocator, app_state->texture_system_memory_requirement);
    // second pass actually initializes the texture system, pass it a pointer to the required memory, and a pointer to where the memory is, and a pointer to the application name
    if (!texture_system_initialize(&app_state->texture_system_memory_requirement, app_state->texture_system_state, texture_sys_config)) {  // if it fails
        KFATAL("Failed to initialize texture system. Application cannot continue");                                                        // throw a fatal error
        return false;                                                                                                                      // and boot out
    }

    // material system
    material_system_config material_sys_config;     // define the material system configuration struct
    material_sys_config.max_material_count = 4096;  // max number of material that can be loaded
    // on the first pass just pass in the pointer to the material system memory requirement field, and the configuration struct, leave the rest 0
    material_system_initialize(&app_state->material_system_memory_requirement, 0, material_sys_config);
    // allocate memory from the linear allocator for the state of material system, and give the pointer to the memory to material system state
    app_state->material_system_state = linear_allocator_allocate(&app_state->systems_allocator, app_state->material_system_memory_requirement);
    // second pass actually initializes the material system, pass it a pointer to the required memory, and a pointer to where the memory is, and a pointer to the application name
    if (!material_system_initialize(&app_state->material_system_memory_requirement, app_state->material_system_state, material_sys_config)) {  // if it fails
        KFATAL("Failed to initialize material system. Application cannot continue");                                                           // throw a fatal error
        return false;                                                                                                                          // and boot out
    }

    // geometry system
    geometry_system_config geometry_sys_config;     // define the geometry system configuration struct
    geometry_sys_config.max_geometry_count = 4096;  // max number of geometry that can be loaded
    // on the first pass just pass in the pointer to the geometry system memory requirement field, and the configuration struct, leave the rest 0
    geometry_system_initialize(&app_state->geometry_system_memory_requirement, 0, geometry_sys_config);
    // allocate memory from the linear allocator for the state of geometry system, and give the pointer to the memory to geometry system state
    app_state->geometry_system_state = linear_allocator_allocate(&app_state->systems_allocator, app_state->geometry_system_memory_requirement);
    // second pass actually initializes the geometry system, pass it a pointer to the required memory, and a pointer to where the memory is, and a pointer to the application name
    if (!geometry_system_initialize(&app_state->geometry_system_memory_requirement, app_state->geometry_system_state, geometry_sys_config)) {  // if it fails
        KFATAL("Failed to initialize geometry system. Application cannot continue");                                                           // throw a fatal error
        return false;                                                                                                                          // and boot out
    }

    // TODO: temp
    // load up a plane configuration, and load geometry from it.
    geometry_config g_config = geometry_system_generate_cube_config(10.0f, 10.0f, 10.0f, 1.0f, 1.0f, "test_cube", "test_material");
    app_state->test_geometry = geometry_system_acquire_from_config(g_config, true);

    // clean up the allocations for the geometry config
    kfree(g_config.vertices, sizeof(vertex_3d) * g_config.vertex_count, MEMORY_TAG_ARRAY);
    kfree(g_config.indices, sizeof(u32) * g_config.index_count, MEMORY_TAG_ARRAY);

    // load up some test ui geometry
    geometry_config ui_config;
    ui_config.vertex_size = sizeof(vertex_3d);
    ui_config.vertex_count = 4;
    ui_config.index_size = sizeof(u32);
    ui_config.index_count = 6;
    string_ncopy(ui_config.material_name, "test_ui_material", MATERIAL_NAME_MAX_LENGTH);
    string_ncopy(ui_config.name, "test_ui_geometry", GEOMETRY_NAME_MAX_LENGTH);

    const f32 w = 128.0f;
    const f32 h = 32.0f;
    vertex_2d uiverts[4];
    uiverts[0].position.x = 0.0f;  // 0    3
    uiverts[0].position.y = 0.0f;  //
    uiverts[0].texcoord.x = 0.0f;  //
    uiverts[0].texcoord.y = 0.0f;  // 2    1

    uiverts[1].position.y = h;
    uiverts[1].position.x = w;
    uiverts[1].texcoord.x = 1.0f;
    uiverts[1].texcoord.y = 1.0f;

    uiverts[2].position.x = 0.0f;
    uiverts[2].position.y = h;
    uiverts[2].texcoord.x = 0.0f;
    uiverts[2].texcoord.y = 1.0f;

    uiverts[3].position.x = w;
    uiverts[3].position.y = 0.0f;
    uiverts[3].texcoord.x = 1.0f;
    uiverts[3].texcoord.y = 0.0f;
    ui_config.vertices = uiverts;

    // indices - counter clockwise
    u32 uiindices[6] = {2, 1, 0, 3, 0, 1};
    ui_config.indices = uiindices;

    // get ui geometry from config
    app_state->test_ui_geometry = geometry_system_acquire_from_config(ui_config, true);

    // load up default geometry
    // app_state->test_geometry = geometry_system_get_default();
    // TODO: end temp

    // initialize the game
    if (!app_state->game_inst->initialize(app_state->game_inst)) {
        KFATAL("Game failed to initialize.")
        return false;
    }

    // call resize once to ensure the proper size has been set.
    // attach event handler for resizing events -- not ready for this to work yet but getting it ready
    app_state->game_inst->on_resize(app_state->game_inst, app_state->width, app_state->height);  // game gets the window dimensions when they change - set the app state to fit the dimensions when changed

    return true;
}

b8 application_run() {
    app_state->is_running = true;  // boolean to say the app is running
    // start the clock for the app
    clock_start(&app_state->clock);                   // start the clock for the app state
    clock_update(&app_state->clock);                  // update the clock for the app state - update the elapsed time
    app_state->last_time = app_state->clock.elapsed;  // set last time to the elapsed time
    // f64 running_time = 0;                             // declare with 0 - to keep track of how much time has accumulated
    u8 frame_count = 0;                    // declare with 0 - to keep track of the frames per second
    f64 target_frame_seconds = 1.0f / 60;  // target frame rate of 60 frames per second - so this gives us a 60th of a second 1/60s - for places where the frame rate may need to be limited

    // test of the memory subsystem
    KINFO(get_memory_usage_str())
    // this is basically the "game" loop at the moment will run as long as app state remains true
    while (app_state->is_running) {
        if (!platform_pump_messages()) {    // if there are no events return false and shut the app doen
            app_state->is_running = false;  // shut down application layer
        }

        if (!app_state->is_suspended) {
            // update clock and get delta time
            clock_update(&app_state->clock);                      // update the elapsed time
            f64 current_time = app_state->clock.elapsed;          // grab the clocks current elapsed time
            f64 delta = (current_time - app_state->last_time);    // create delta by taking the current time and subtracting from it the last time
            f64 frame_start_time = platform_get_absolute_time();  // get the time from the os and set it to frame start time - to keep track of how long each frame takes to render

            if (!app_state->game_inst->update(app_state->game_inst, (f32)delta)) {  // run the update routine. the zero is in polace of delta time for now, will be fixed later
                KFATAL("Game update failed, shutting down.");
                app_state->is_running = false;  // shut down the application layer
                break;
            }

            // call the games render routine
            if (!app_state->game_inst->render(app_state->game_inst, (f32)delta)) {  // again the zero is in place of delta time
                KFATAL("Game renderer failed, shutting down");
                app_state->is_running = false;
                break;
            }

            // this is not how this will be done in the future
            // TODO: refactor packet creation
            render_packet packet;
            packet.delta_time = delta;

            // TODO: temp
            geometry_render_data test_render;
            test_render.geometry = app_state->test_geometry;
            test_render.model = mat4_identity();
            static f32 angle = 0;
            angle += (1.0f * delta);
            quat rotation = quat_from_axis_angle((vec3){0, 1, 0}, angle, true);
            test_render.model = quat_to_mat4(rotation);  //  quat_to_rotation_matrix(rotation, vec3_zero());

            packet.geometry_count = 1;
            packet.geometries = &test_render;

            geometry_render_data test_ui_render;
            test_ui_render.geometry = app_state->test_ui_geometry;
            test_ui_render.model = mat4_translation((vec3){0, 0, 0});
            packet.ui_geometry_count = 1;
            packet.ui_geometries = &test_ui_render;
            // TODO: end temp

            renderer_draw_frame(&packet);  // here is where the draw calls are going to be?

            // figure out how long the frame took and, if below
            f64 frame_end_time = platform_get_absolute_time();           // get time from os and set to frame end time
            f64 frame_elapsed_time = frame_end_time - frame_start_time;  // get the elapsed time from the start and end times
            // running_time += frame_elapsed_time;                                 // increment the running time by the frame elapsed time
            f64 remaining_seconds = target_frame_seconds - frame_elapsed_time;  // take the elapsed time away from one second

            if (remaining_seconds > 0) {                        // if there are still ms left
                u64 remaining_ms = (remaining_seconds * 1000);  // conver to ms

                // if there is time left, give it back to the os - helps with performance
                b8 limit_frames = false;  // a switch for enabling the below statement
                if (remaining_ms > 0 && limit_frames) {
                    platform_sleep(remaining_ms - 1);
                }

                frame_count++;  // increment the frame count
            }

            // NOTE: input update/state copying should always be handled after any input should be recorder, i.e. before this line
            // as a safety, input is the last thing to be updated before this frame ends
            input_update(delta);

            // update last time
            app_state->last_time = current_time;  // at the very end set last time to the current time
        }
    }

    app_state->is_running = false;  // in the event it exits the loop while true make sure it shutsdown

    // event listeners - unregister
    event_unregister(EVENT_CODE_APPLICATION_QUIT, 0, application_on_event);
    event_unregister(EVENT_CODE_KEY_PRESSED, 0, application_on_key);
    event_unregister(EVENT_CODE_KEY_RELEASED, 0, application_on_key);
    event_unregister(EVENT_CODE_RESIZED, 0, application_on_resized);
    // TODO: temp
    event_unregister(EVENT_CODE_DEBUG0, 0, event_on_debug_event);
    // TODO: end temp

    // shutdown the input system  -  pass it the pointer to where the state is being stored
    input_system_shutdown(app_state->input_system_state);

    // shutdown the geometry system - pass it a pointer to where the state is being stored
    geometry_system_shutdown(app_state->geometry_system_state);

    // shutdown the material system - pass it a pointer to where the state is being stored
    material_system_shutdown(app_state->material_system_state);

    // shutdown the texture system - pass it a pointer to where the state is being stored
    texture_system_shutdown(app_state->texture_system_state);

    // shutdown the shader system - pass it a pointer to where the state is being stored
    shader_system_shutdown(app_state->shader_system_state);

    // shutdown the renderer  -  pass it the pointer to where the state is being stored
    renderer_system_shutdown(app_state->renderer_system_state);

    // shut down the resource system
    resource_system_shutdown(app_state->resource_system_state);

    // shut down the platform layer -  pass it the pointer to where the state is being stored
    platform_system_shutdown(app_state->platform_system_state);

    // shutdown the event system, pass in a pointer to the event system state
    event_system_shutdown(app_state->event_system_state);

    // shut down the memory subsystem - pass it the pointer to where the state is being stored
    memory_system_shutdown();

    return true;
}

// a way to pass the window size to the renderer, passes out a width and height
void application_get_framebuffer_size(u32* width, u32* height) {
    *width = app_state->width;    // pass the app state width to width
    *height = app_state->height;  // pass the app state height to height
}

b8 application_on_event(u16 code, void* sender, void* listener_inst, event_context context) {
    switch (code) {                          // chaeck to see what code was passed
        case EVENT_CODE_APPLICATION_QUIT: {  // if code is app quit
            KINFO("EVENT_CODE_APPLICATION_QUIT recieved, shutting down.\n");
            app_state->is_running = false;  // switch off the app
            return true;                    // blocks the app quit from going anywhere else
        }
    }

    return false;  // if the code not in the list
}

b8 application_on_key(u16 code, void* sender, void* listener_inst, event_context context) {
    if (code == EVENT_CODE_KEY_PRESSED) {    // was it a key press
        u16 key_code = context.data.u16[0];  // set key code to the key data in context
        if (key_code == KEY_ESCAPE) {        // if the key is esc
            // NOTE: technically firing an event to itself, but eventually there may be other listeners
            event_context data = {};                           // create empty context, data
            event_fire(EVENT_CODE_APPLICATION_QUIT, 0, data);  // fire the event

            // block anything else from processing this
            return true;
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
    return false;
}

// on resized event, pass in the code, a pointer to the sender, a pointer to the instance, and the context - this is an event handler
b8 application_on_resized(u16 code, void* sender, void* listener_inst, event_context context) {
    if (code == EVENT_CODE_RESIZED) {      // if it is a resized event
        u16 width = context.data.u16[0];   // from the u16 array in context data get the width from index 0
        u16 height = context.data.u16[1];  // and the height from index 1

        // check if different. if so, trigger a resize event
        if (width != app_state->width || height != app_state->height) {
            app_state->width = width;    // pass in the width from the context
            app_state->height = height;  // pass in the height from the context

            KDEBUG("Window resize: %i, %i", width, height);  // display the dimendions in a debug msg

            // handle minimization
            if (width == 0 || height == 0) {                                // if either the width or the height are 0
                KINFO("Window is minimized, suspending the application.");  // throw a message
                app_state->is_suspended = true;                             // set the app state to suspended
                return true;
            } else {                                                  // if width and height arent 0
                if (app_state->is_suspended) {                        // if app state is suspended
                    KINFO("Window restored, resuming application.");  // throw an info msg
                    app_state->is_suspended = false;                  // ans set the app state to not suspended
                }
                app_state->game_inst->on_resize(app_state->game_inst, width, height);  // call the function pointer on resize, pass in  the game inst, and the dimaensions
                renderer_on_resized(width, height);                                    // call the renderer on resized function, and pass through the width and height
            }
        }
    }

    // event purposefully not handled to allow other listeners to get this
    return false;
}
