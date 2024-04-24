#pragma once

#include "math/math_types.h"

// @brief represents a camera that can be used for a variety of things, especially
// rendering. ideally these will be created and managed by a camera system
typedef struct camera {
    // @brief the position of this camera.
    // NOTE: do not set this directly, use camera_position_set() instead so the
    // view matrix is recalculated when needed
    vec3 position;
    // @brief the rotation of this camera using euler angles (pitch, yaw, roll)
    // NOTE: do not set this directly, use camera_rotation_euler_set() instead so the
    // view matrix is recalculated when needed
    vec3 euler_rotation;
    // @brief internal flag used to determine when the view matrix needs to be rebuilt
    b8 is_dirty;

    // @brief the view matrix of this camera
    // NOTE: IMPORTANT: do not get this directly, use camera_view_get() instead so the
    // view matrix is recalculated when needed
    mat4 view_matrix;
} camera;

// @brief creates a new camera with default zero position and rotation, and view
// identity matrix. ideally, the camera system should be used to create this
// rather than doing it directly
// @return a copy of a newly created camera
KAPI camera camera_create();

// @brief defaults the provided camera to default zero rotation and position,
// and view matrix to identity
// @param c a pointer to the camera to reset
KAPI void camera_reset(camera* c);

// @brief gets a copy of the camera's position
// @param c a constant pointer to a camera
// @return a copy of the camera's position
KAPI vec3 camera_position_get(const camera* c);

// @brief sets the provided camera's position
// @param c a pointer to a camera
// @param position the position to be set
KAPI void camera_position_set(camera* c, vec3 position);

// @brief gets a copy of the camera's rotation in euler angles
// @param c a constant pointer to a camera
// @return a copy of the camera's rotation in euler angles
KAPI vec3 camera_rotation_euler_get(const camera* c);

// @brief sets the provided camera's rotation in euler angles
// @param c a pointer to a camera
// @param rotation the rotation in euler angles to be set
KAPI void camera_rotation_euler_set(camera* c, vec3 rotation);

// @brief gets a copy of the camera's view matrix. if the camera is dirty,
// a new one is created, set and returned.
// @param c a constant pointer to a camera
// @return a copy of the up to date view matrix
KAPI mat4 camera_view_get(camera* c);

// @brief returns a copy of the camera's forward vector
// @param c a constant pointer to a camera
// @return a copy of the camera's forward vector
KAPI vec3 camera_forward(camera* c);

// @brief returns a copy of the camera's backward vector
// @param c a constant pointer to a camera
// @return a copy of the camera's backward vector
KAPI vec3 camera_backward(camera* c);

// @brief returns a copy of the camera's left vector
// @param c a constant pointer to a camera
// @return a copy of the camera's left vector
KAPI vec3 camera_left(camera* c);

// @brief returns a copy of the camera's right vector
// @param c a constant pointer to a camera
// @return a copy of the camera's right vector
KAPI vec3 camera_right(camera* c);

// @brief moves the camera forward by the given amount
// @param c a constant pointer to a camera
// @return amount the amount to move
KAPI void camera_move_forward(camera* c, f32 amount);

// @brief moves the camera backward by the given amount
// @param c a constant pointer to a camera
// @return amount the amount to move
KAPI void camera_move_backward(camera* c, f32 amount);

// @brief moves the camera left by the given amount
// @param c a constant pointer to a camera
// @return amount the amount to move
KAPI void camera_move_left(camera* c, f32 amount);

// @brief moves the camera right by the given amount
// @param c a constant pointer to a camera
// @return amount the amount to move
KAPI void camera_move_right(camera* c, f32 amount);

// @brief moves the camera up (straight along the y axis) by the given amount
// @param c a constant pointer to a camera
// @return amount the amount to move
KAPI void camera_move_up(camera* c, f32 amount);

// @brief moves the camera down (straight along the y axis) by the given amount
// @param c a constant pointer to a camera
// @return amount the amount to move
KAPI void camera_move_down(camera* c, f32 amount);

// @brief adjusts the camera's yaw by the given amount
// @param c a constant pointer to a camera
// @return amount the amount to asjust by
KAPI void camera_yaw(camera* c, f32 amount);

// @brief adjusts the camera's pitch by the given amount
// @param c a constant pointer to a camera
// @return amount the amount to asjust by
KAPI void camera_pitch(camera* c, f32 amount);