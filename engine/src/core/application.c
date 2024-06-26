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
#include "systems/camera_system.h"
#include "systems/render_view_system.h"

// TODO: temp
#include "math/kmath.h"
#include "math/transform.h"
#include "math/geometry_utils.h"
#include "containers/darray.h"
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

    u64 renderer_view_system_memory_requirement;
    void* renderer_view_system_state;

    // texture system state allocation
    u64 texture_system_memory_requirement;  // where the amount of storage that is needed fot the texture system is stored
    void* texture_system_state;             // a pointer to where the texture system state is being stored

    // material system state allocation
    u64 material_system_memory_requirement;  // where the amount of storage that is needed fot the material system is stored
    void* material_system_state;             // a pointer to where the material system state is being stored

    // geometry system state allocation
    u64 geometry_system_memory_requirement;  // where the amount of storage that is needed fot the geometry system is stored
    void* geometry_system_state;             // a pointer to where the geometry system state is being stored

    u64 camera_system_memory_requirement;
    void* camera_system_state;

    // TODO: temp
    skybox sb;

    // darray
    mesh meshes[10];
    u32 mesh_count;

    mesh ui_meshes[10];
    u32 ui_mesh_count;
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

    // save off the old names
    const char* old_name = names[choice];

    choice++;     // increment
    choice %= 3;  // then mod back to 0. still need to learn this

    // just swap out the material on the first mesh if it exists
    geometry* g = app_state->meshes[0].geometries[0];
    if (g) {
        // acquire the new material
        g->material = material_system_acquire(names[choice]);
        if (!g->material) {
            KWARN("event_on_debug_event no material found! Using default material.");
            g->material = material_system_get_default();
        }

        // release the old diffuse material
        material_system_release(old_name);
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

    // Camera
    camera_system_config camera_sys_config;
    camera_sys_config.max_camera_count = 61;
    camera_system_initialize(&app_state->camera_system_memory_requirement, 0, camera_sys_config);
    app_state->camera_system_state = linear_allocator_allocate(&app_state->systems_allocator, app_state->camera_system_memory_requirement);
    if (!camera_system_initialize(&app_state->camera_system_memory_requirement, app_state->camera_system_state, camera_sys_config)) {
        KFATAL("Failed to initialize camera system. Application cannot continue.");
        return false;
    }

    render_view_system_config render_view_sys_config = {};
    render_view_sys_config.max_view_count = 251;
    render_view_system_initialize(&app_state->renderer_view_system_memory_requirement, 0, render_view_sys_config);
    app_state->renderer_view_system_state = linear_allocator_allocate(&app_state->systems_allocator, app_state->renderer_view_system_memory_requirement);
    if (!render_view_system_initialize(&app_state->renderer_view_system_memory_requirement, app_state->renderer_view_system_state, render_view_sys_config)) {
        KFATAL("Failed to initialize render view system. Aborting application.");
        return false;
    }

    // load render views
    render_view_config skybox_config = {};
    skybox_config.type = RENDERER_VIEW_KNOWN_TYPE_SKYBOX;
    skybox_config.width = 0;
    skybox_config.height = 0;
    skybox_config.name = "skybox";
    skybox_config.pass_count = 1;
    render_view_pass_config skybox_passes[1];
    skybox_passes[0].name = "Renderpass.Builtin.Skybox";
    skybox_config.passes = skybox_passes;
    skybox_config.view_matrix_source = RENDER_VIEW_VIEW_MATRIX_SOURCE_SCENE_CAMERA;
    if (!render_view_system_create(&skybox_config)) {
        KFATAL("Failed to create skybox view. Aborting application.");
        return false;
    }

    render_view_config opaque_world_config = {};
    opaque_world_config.type = RENDERER_VIEW_KNOWN_TYPE_WORLD;
    opaque_world_config.width = 0;
    opaque_world_config.height = 0;
    opaque_world_config.name = "world_opaque";
    opaque_world_config.pass_count = 1;
    render_view_pass_config passes[1];
    passes[0].name = "Renderpass.Builtin.World";
    opaque_world_config.passes = passes;
    opaque_world_config.view_matrix_source = RENDER_VIEW_VIEW_MATRIX_SOURCE_SCENE_CAMERA;
    if (!render_view_system_create(&opaque_world_config)) {
        KFATAL("Failed to create view. Aborting application.");
        return false;
    }

    render_view_config ui_view_config = {};
    ui_view_config.type = RENDERER_VIEW_KNOWN_TYPE_UI;
    ui_view_config.width = 0;
    ui_view_config.height = 0;
    ui_view_config.name = "ui";
    ui_view_config.pass_count = 1;
    render_view_pass_config ui_passes[1];
    ui_passes[0].name = "Renderpass.Builtin.UI";
    ui_view_config.passes = ui_passes;
    ui_view_config.view_matrix_source = RENDER_VIEW_VIEW_MATRIX_SOURCE_SCENE_CAMERA;
    if (!render_view_system_create(&ui_view_config)) {
        KFATAL("Failed to create view. Aborting application.");
        return false;
    }

    // TODO: temp

    // skybox
    texture_map* cube_map = &app_state->sb.cubemap;
    cube_map->filter_magnify = cube_map->filter_minify = TEXTURE_FILTER_MODE_LINEAR;
    cube_map->repeat_u = cube_map->repeat_v = cube_map->repeat_w = TEXTURE_REPEAT_CLAMP_TO_EDGE;
    cube_map->use = TEXTURE_USE_MAP_CUBEMAP;
    if (!renderer_texture_map_acquire_resources(cube_map)) {
        KFATAL("Unable to acquire resources for cube map texture.");
        return false;
    }
    cube_map->texture = texture_system_acquire_cube("skybox", true);
    geometry_config skybox_cube_config = geometry_system_generate_cube_config(10.0f, 10.0f, 10.0f, 1.0f, 1.0f, "skybox_cube", 0);
    // clear out the material name
    skybox_cube_config.material_name[0] = 0;
    app_state->sb.g = geometry_system_acquire_from_config(skybox_cube_config, true);
    app_state->sb.render_frame_number = INVALID_ID_U64;
    shader* skybox_shader = shader_system_get(BUILTIN_SHADER_NAME_SKYBOX);
    texture_map* maps[1] = {&app_state->sb.cubemap};
    if (!renderer_shader_acquire_instance_resources(skybox_shader, maps, &app_state->sb.instance_id)) {
        KFATAL("Unable to acquire shader resources for skybox texture.");
        return false;
    }

    // world meshes
    app_state->mesh_count = 0;

    // load up a cube configuration, and load geometry from it.
    mesh* cube_mesh = &app_state->meshes[app_state->mesh_count];
    cube_mesh->geometry_count = 1;
    cube_mesh->geometries = kallocate(sizeof(mesh*) * cube_mesh->geometry_count, MEMORY_TAG_ARRAY);
    geometry_config g_config = geometry_system_generate_cube_config(10.0f, 10.0f, 10.0f, 1.0f, 1.0f, "test_cube", "test_material");
    cube_mesh->geometries[0] = geometry_system_acquire_from_config(g_config, true);
    cube_mesh->transform = transform_create();
    app_state->mesh_count++;

    // clean up the allocations for the geometry config
    geometry_system_config_dispose(&g_config);

    // a second cube
    mesh* cube_mesh_2 = &app_state->meshes[app_state->mesh_count];
    cube_mesh_2->geometry_count = 1;
    cube_mesh_2->geometries = kallocate(sizeof(mesh*) * cube_mesh_2->geometry_count, MEMORY_TAG_ARRAY);
    g_config = geometry_system_generate_cube_config(5.0f, 5.0f, 5.0f, 1.0f, 1.0f, "test_cube_2", "test_material");
    cube_mesh_2->geometries[0] = geometry_system_acquire_from_config(g_config, true);
    cube_mesh_2->transform = transform_from_position((vec3){10.0f, 0.0f, 1.0f});
    // set the first cube as the parent to the second
    transform_set_parent(&cube_mesh_2->transform, &cube_mesh->transform);
    app_state->mesh_count++;
    // clean up the allocations for the geometry config
    geometry_system_config_dispose(&g_config);

    // a third cube!
    mesh* cube_mesh_3 = &app_state->meshes[app_state->mesh_count];
    cube_mesh_3->geometry_count = 1;
    cube_mesh_3->geometries = kallocate(sizeof(mesh*) * cube_mesh_3->geometry_count, MEMORY_TAG_ARRAY);
    g_config = geometry_system_generate_cube_config(2.0f, 2.0f, 2.0f, 1.0f, 1.0f, "test_cube_2", "test_material");
    cube_mesh_3->geometries[0] = geometry_system_acquire_from_config(g_config, true);
    cube_mesh_3->transform = transform_from_position((vec3){5.0f, 0.0f, 1.0f});
    // set the second cube as the parent to the third
    transform_set_parent(&cube_mesh_3->transform, &cube_mesh_2->transform);
    app_state->mesh_count++;
    // clean up the allocations for the geometry config
    geometry_system_config_dispose(&g_config);

    // a test mesh loaded from file
    mesh* car_mesh = &app_state->meshes[app_state->mesh_count];
    resource car_mesh_resource = {};
    if (!resource_system_load("falcon", RESOURCE_TYPE_MESH, 0, &car_mesh_resource)) {
        KERROR("Failed to load car test mesh!");
    } else {
        geometry_config* configs = (geometry_config*)car_mesh_resource.data;
        car_mesh->geometry_count = car_mesh_resource.data_size;
        car_mesh->geometries = kallocate(sizeof(geometry*) * car_mesh->geometry_count, MEMORY_TAG_ARRAY);
        for (u32 i = 0; i < car_mesh->geometry_count; ++i) {
            car_mesh->geometries[i] = geometry_system_acquire_from_config(configs[i], true);
        }
        car_mesh->transform = transform_from_position((vec3){15.0f, 0.0f, 1.0f});
        resource_system_unload(&car_mesh_resource);
        app_state->mesh_count++;
    }

    // a test mesh loaded from file
    mesh* sponza_mesh = &app_state->meshes[app_state->mesh_count];
    resource sponza_mesh_resource = {};
    if (!resource_system_load("sponza", RESOURCE_TYPE_MESH, 0, &sponza_mesh_resource)) {
        KERROR("Failed to load sponza mesh!");
    } else {
        geometry_config* sponza_configs = (geometry_config*)sponza_mesh_resource.data;
        sponza_mesh->geometry_count = sponza_mesh_resource.data_size;
        sponza_mesh->geometries = kallocate(sizeof(mesh*) * sponza_mesh->geometry_count, MEMORY_TAG_ARRAY);
        for (u32 i = 0; i < sponza_mesh->geometry_count; ++i) {
            sponza_mesh->geometries[i] = geometry_system_acquire_from_config(sponza_configs[i], true);
        }
        sponza_mesh->transform = transform_from_position_rotation_scale((vec3){15.0f, 0.0f, 1.0f}, quat_identity(), (vec3){0.05f, 0.05f, 0.05f});
        resource_system_unload(&sponza_mesh_resource);
        app_state->mesh_count++;
    }

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
    app_state->ui_mesh_count = 1;
    app_state->ui_meshes[0].geometry_count = 1;
    app_state->ui_meshes[0].geometries = kallocate(sizeof(geometry*), MEMORY_TAG_ARRAY);
    app_state->ui_meshes[0].geometries[0] = geometry_system_acquire_from_config(ui_config, true);
    app_state->ui_meshes[0].transform = transform_create();

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

            if (app_state->mesh_count > 0) {
                // perform a small rotation on the first mesh
                quat rotation = quat_from_axis_angle((vec3){0, 1, 0}, 0.5f * delta, false);
                transform_rotate(&app_state->meshes[0].transform, rotation);

                // perform a similar rotation on the second mesh, if it exists.
                if (app_state->mesh_count > 1) {
                    transform_rotate(&app_state->meshes[1].transform, rotation);
                }

                // perform a similar rotation on the third mesh, if it exists.
                if (app_state->mesh_count > 2) {
                    transform_rotate(&app_state->meshes[2].transform, rotation);
                }
            }

            // TODO: refactor packet creation
            render_packet packet = {};
            packet.delta_time = delta;

            // TODO: read from frame config
            packet.view_count = 3;
            render_view_packet views[3];
            kzero_memory(views, sizeof(render_view_packet) * packet.view_count);
            packet.views = views;

            // skybox
            skybox_packet_data skybox_data = {};
            skybox_data.sb = &app_state->sb;
            if (!render_view_system_build_packet(render_view_system_get("skybox"), &skybox_data, &packet.views[0])) {
                KERROR("Failed to build packet for view 'skybox'.");
                return false;
            }

            // world
            mesh_packet_data world_mesh_data = {};
            world_mesh_data.mesh_count = app_state->mesh_count;
            world_mesh_data.meshes = app_state->meshes;
            // TODO: performs a lookup on every frame
            if (!render_view_system_build_packet(render_view_system_get("world_opaque"), &world_mesh_data, &packet.views[1])) {
                KERROR("Failed to build packet for view 'world_opaque'.");
                return false;
            }

            // ui
            mesh_packet_data ui_mesh_data = {};
            ui_mesh_data.mesh_count = app_state->ui_mesh_count;
            ui_mesh_data.meshes = app_state->ui_meshes;
            if (!render_view_system_build_packet(render_view_system_get("ui"), &ui_mesh_data, &packet.views[2])) {
                KERROR("Failed to build packet for view 'ui'.");
                return false;
            }
            // TODO: end temp

            renderer_draw_frame(&packet);  // here is where the draw calls are going to be?

            // TODO: temp
            // clean up the packet

            // TODO: end temp

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

    // TODO: temp
    // TODO: implement skybox destroy
    renderer_texture_map_release_resources(&app_state->sb.cubemap);
    // TODO: end temp

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
