#include "game.h"

#include <core/logger.h>
#include <core/kmemory.h>

#include <core/input.h>
#include <core/event.h>

#include <math/kmath.h>
#include <renderer/renderer_types.inl>

b8 game_initialize(game* game_inst) {
    KDEBUG("game initialize() called!");

    // store the game state in an easier form to call
    game_state* state = (game_state*)game_inst->state;

    state->world_camera = camera_system_get_default();
    camera_position_set(state->world_camera, (vec3){10.5f, 5.0f, 9.5f});

    return true;
}

b8 game_update(game* game_inst, f32 delta_time) {
    static u64 alloc_count = 0;              // define alloc count - static variable scoped to this function
    u64 prev_alloc_count = alloc_count;      // set previous alloc count to previous alloc count
    alloc_count = get_memory_alloc_count();  // store the dynamic allocation count in alloc count
    if (input_is_key_up('M') && input_was_key_down('M')) {
        KDEBUG("allocations: %llu (%llu this frame", alloc_count, alloc_count - prev_alloc_count);
    }

    // TODO: temporary
    if (input_is_key_up('T') && input_was_key_down('T')) {
        KDEBUG("Swapping texture!");
        event_context context = {};
        event_fire(EVENT_CODE_DEBUG0, game_inst, context);
    }
    // TODO: end temporary

    // store the game state in an easier form to call
    game_state* state = (game_state*)game_inst->state;

    // HACK: temp hack to move the camera around
    //  - if the a key or the left key get pressed
    if (input_is_key_down('A') || input_is_key_down(KEY_LEFT)) {
        camera_yaw(state->world_camera, 1.0f * delta_time);  // change the yaw turning the camera to the left
    }

    //  - if the d key or the right key get pressed
    if (input_is_key_down('D') || input_is_key_down(KEY_RIGHT)) {
        camera_yaw(state->world_camera, -1.0f * delta_time);  // change the yaw turning the camera to the right
    }

    //  - if the up key get pressed
    if (input_is_key_down(KEY_UP)) {
        camera_pitch(state->world_camera, 1.0f * delta_time);  // change the pitch tilting the camera up
    }

    //  - if the down key get pressed
    if (input_is_key_down(KEY_DOWN)) {
        camera_pitch(state->world_camera, -1.0f * delta_time);  // change the pitch tilting the camera down
    }

    // for controlling the camera's movement speed
    static const f32 temp_move_speed = 50.0f;

    //  - if the w key gets pressed
    if (input_is_key_down('W')) {
        camera_move_forward(state->world_camera, temp_move_speed * delta_time);
    }

    //  - if the S key gets pressed
    if (input_is_key_down('S')) {
        camera_move_backward(state->world_camera, temp_move_speed * delta_time);
    }

    //  - if the w key gets pressed
    if (input_is_key_down('Q')) {
        camera_move_left(state->world_camera, temp_move_speed * delta_time);
    }

    //  - if the S key gets pressed
    if (input_is_key_down('E')) {
        camera_move_right(state->world_camera, temp_move_speed * delta_time);
    }

    // if the space key is pressed
    if (input_is_key_down(KEY_SPACE)) {
        camera_move_up(state->world_camera, temp_move_speed * delta_time);
    }

    // if the x key is pressed
    if (input_is_key_down('X')) {
        camera_move_down(state->world_camera, temp_move_speed * delta_time);
    }

    // TODO: temp
    if (input_is_key_up('P') && input_was_key_down('P')) {
        KDEBUG(
            "Pos:[%.2f, %.2f, %.2f",
            state->world_camera->position.x,
            state->world_camera->position.y,
            state->world_camera->position.z);
    }

    // RENDERER DEBUG FUNCTIONS
    if (input_is_key_up('1') && input_was_key_down('1')) {
        event_context data = {};
        data.data.i32[0] = RENDERER_VIEW_MODE_LIGHTING;
        event_fire(EVENT_CODE_SET_RENDER_MODE, game_inst, data);
    }

    if (input_is_key_up('2') && input_was_key_down('2')) {
        event_context data = {};
        data.data.i32[0] = RENDERER_VIEW_MODE_NORMALS;
        event_fire(EVENT_CODE_SET_RENDER_MODE, game_inst, data);
    }

    if (input_is_key_up('0') && input_was_key_down('0')) {
        event_context data = {};
        data.data.i32[0] = RENDERER_VIEW_MODE_DEFAULT;
        event_fire(EVENT_CODE_SET_RENDER_MODE, game_inst, data);
    }
    // TODO: end temp

    return true;
}

b8 game_render(game* game_inst, f32 delta_time) {
    return true;
}

void game_on_resize(game* game_inst, u32 width, u32 height) {
}