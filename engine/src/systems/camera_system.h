
#pragma once

#include "renderer/camera.h"

// @brief the camera system configuration
typedef struct camera_system_config {
    // @brief NOTE: the maximum number of cameras that can be managed by the system
    u16 max_camera_count;
} camera_system_config;

// @brief the name of the default camera
#define DEFAULT_CAMERA_NAME "default"

// @brief initializes the camera system. should be called twice, once to get the memory requirement
// (passing state=0), and a second time passing an block of memory to actually initialize the system
// @param memory_requirement a pointer to hold the memory requirement as it is calculated
// @param state a block of memory to hold the state or 0 if getting the memory requirement
// @param config the configuration for this system
// @return true on success, otherwise false
b8 camera_system_initialize(u64* memory_requirement, void* state, camera_system_config config);

// @brief shuts down the geometry camera
// @param state the state block of memory
void camera_system_shutdown(void* state);

// @brief acquires a pointer to a camera by name. if one is not found, a new one is created and
// returned.  internal reference counter is incremented
// @param name the name of the camera to acquire
// @return a pointer to a camera if successful, 0 if an error occurs
KAPI camera* camera_system_acquire(const char* name);

// @brief releases a camera with the given name. internal reference counter is decremented. if this
// reaches 0, the camera is reset, and the reference is unusable by a new camera
// @param name the name of the cmaera to be released
KAPI void camera_system_release(const char* name);

// @brief gets a pointer to the default camera
// @return a pointer to the default camera
KAPI camera* camera_system_get_default();
