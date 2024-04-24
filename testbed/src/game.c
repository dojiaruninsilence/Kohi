#include "game.h"

#include <core/logger.h>
#include <core/kmemory.h>

#include <core/input.h>
#include <core/event.h>

#include <math/kmath.h>

// HACK: this should not be exposed outside the engine
#include <renderer/renderer_frontend.h>

void recalculate_view_matrix(game_state* state) {
    if (state->camera_view_dirty) {                                                                           // this is an indication if the view matrix needs to be calculated, because of a move, resize event or anything else that may change the camera
        mat4 rotation = mat4_euler_xyz(state->camera_euler.x, state->camera_euler.y, state->camera_euler.z);  // using onlu euler rotations for now
        mat4 translation = mat4_translation(state->camera_position);                                          // multiply by the translation and rotation

        state->view = mat4_mul(rotation, translation);  // this order is also imortant, scale would go last if we needed one, dont need scale for camera
        state->view = mat4_inverse(state->view);        // camera views need to be the inverse of the actual rotation and position(cause technically its the cameras view, not the camera we care about)

        state->camera_view_dirty = false;  // reset so calcualtions dont run until the next change
    }
}

void camera_yaw(game_state* state, f32 amount) {
    state->camera_euler.y += amount;  // increase euler y by the amount input
    state->camera_view_dirty = true;  // set dirty to true, so the view gets recalculated
}

void camera_pitch(game_state* state, f32 amount) {
    state->camera_euler.x += amount;  // increase euler x by the amount input

    // clamp to avoid gimball lock
    f32 limit = deg_to_rad(89.0f);
    state->camera_euler.x = KCLAMP(state->camera_euler.x, -limit, limit);

    state->camera_view_dirty = true;  // set dirty to true, so the view gets recalculated
}

b8 game_initialize(game* game_inst) {
    KDEBUG("game initialize() called!");

    // store the game state in an easier form to call
    game_state* state = (game_state*)game_inst->state;

    // define the camera's default position and rotation
    state->camera_position = (vec3){10.5f, 5.0f, 9.5f};
    state->camera_euler = vec3_zero();

    // caluculate the initial position of the camera
    state->view = mat4_translation(state->camera_position);
    state->view = mat4_inverse(state->view);
    state->camera_view_dirty = true;  // call for a recalulation to calculate the cameras initial rotation

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
        camera_yaw(state, 1.0f * delta_time);  // change the yaw turning the camera to the left
    }

    //  - if the d key or the right key get pressed
    if (input_is_key_down('D') || input_is_key_down(KEY_RIGHT)) {
        camera_yaw(state, -1.0f * delta_time);  // change the yaw turning the camera to the right
    }

    //  - if the up key get pressed
    if (input_is_key_down(KEY_UP)) {
        camera_pitch(state, 1.0f * delta_time);  // change the pitch tilting the camera up
    }

    //  - if the down key get pressed
    if (input_is_key_down(KEY_DOWN)) {
        camera_pitch(state, -1.0f * delta_time);  // change the pitch tilting the camera down
    }

    // for controlling the camera's movement speed
    f32 temp_move_speed = 50.0f;
    vec3 velocity = vec3_zero();

    //  - if the w key gets pressed
    if (input_is_key_down('W')) {
        // dont want to just move forward on the z axis but want to move foreward in relation to the cameras rotation, so..
        vec3 forward = mat4_forward(state->view);  // get the view matrix's forward with the forward function
        velocity = vec3_add(velocity, forward);    // and add it the velocity, so we are now going to be increasing the velocity of the forward direction
    }

    //  - if the S key gets pressed
    if (input_is_key_down('S')) {
        // dont want to just move backward on the z axis but want to move backward in relation to the cameras rotation, so..
        vec3 backward = mat4_backward(state->view);  // get the view matrix's backward with the backward function
        velocity = vec3_add(velocity, backward);     // and add it the velocity, so we are now going to be increasing the velocity of the backward direction
    }

    //  - if the w key gets pressed
    if (input_is_key_down('Q')) {
        // dont want to just move left on the z axis but want to move left in relation to the cameras rotation, so..
        vec3 left = mat4_left(state->view);   // get the view matrix's left with the left function
        velocity = vec3_add(velocity, left);  // and add it the velocity, so we are now going to be increasing the velocity of the left direction
    }

    //  - if the S key gets pressed
    if (input_is_key_down('E')) {
        // dont want to just move right on the z axis but want to move right in relation to the cameras rotation, so..
        vec3 right = mat4_right(state->view);  // get the view matrix's right with the right function
        velocity = vec3_add(velocity, right);  // and add it the velocity, so we are now going to be increasing the velocity of the right direction
    }

    // if the space key is pressed
    if (input_is_key_down(KEY_SPACE)) {
        velocity.y += 1.0f;
    }

    // if the x key is pressed
    if (input_is_key_down('X')) {
        velocity.y -= 1.0f;
    }

    // check if there is a difference in the velocity - using the vec3 compare function, with a tolerance of 0.0002
    vec3 z = vec3_zero();
    if (!vec3_compare(z, velocity, 0.0002f)) {
        // be sure to normalize the velocity before applying the speed
        vec3_normalize(&velocity);

        // set the camera to new positions
        state->camera_position.x += velocity.x * temp_move_speed * delta_time;
        state->camera_position.y += velocity.y * temp_move_speed * delta_time;
        state->camera_position.z += velocity.z * temp_move_speed * delta_time;

        // set the view to be recalulated
        state->camera_view_dirty = true;
    }

    // make sure the view matrix is calulated properly before setting it to view
    recalculate_view_matrix(state);

    // HACK: this should not be exposed outside the engine
    renderer_set_view(state->view, state->camera_position);

    // TODO: temp
    if (input_is_key_up('P') && input_was_key_down('P')) {
        KDEBUG(
            "Pos:[%.2f, %.2f, %.2f",
            state->camera_position.x,
            state->camera_position.y,
            state->camera_position.z);
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