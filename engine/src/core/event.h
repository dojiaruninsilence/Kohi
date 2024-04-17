#pragma once

#include "defines.h"

// this is allocated on the stack
typedef struct event_context {
    // 128 bytes - maximum
    union {
        i64 i64[2];
        u64 u64[2];
        f64 f64[2];

        i32 i32[4];
        u32 u32[4];
        f32 f32[4];

        i16 i16[8];
        u16 u16[8];

        i8 i8[16];
        u8 u8[16];

        char c[16];
    } data;

} event_context;

// should return true if handled
typedef b8 (*PFN_on_event)(u16 code, void* sender, void* listener_inst, event_context data);  // pfn - pointer function. takes in number that ids message explicitly, pointer to the sender, pass the listener instance, then the data

// initialize and shutdown the system - not exported, for engine only
// initialize the event subsystem, - always call twice - on first pass pass in the memory requirement to get the memory required, and zero for the state
// on the second pass - pass in the state as well as the memory rewuirement and actually initialize the subsystem
void event_system_initialize(u64* memory_requirement, void* state);
void event_system_shutdown(void* state);

// register to the listener for when events are sent with the provided code.  events with duplicate listener/callback combos will not be registered again and will cause this to return false
// @param code the evet code to listen for
// @param listener a pointer to a listerner instance, can be null
// @param on_event the callback function pointer to be invoked when the event code is fired
// @returns true if the event is successfully registered; otherwise false
KAPI b8 event_register(u16 code, void* listener, PFN_on_event on_event);  // code to register for, pointer to the listener, pointer to the actual method to be called

// unregister from listening for when events are sent with the provided code. if no matching registration is found, this function returns false.
// @param code the evet code to stop listening for
// @param listener a pointer to a listerner instance, can be null
// @param on_event the callback function pointer to be unregistered
// @returns true if the event is successfully unregistered; otherwise false
KAPI b8 event_unregister(u16 code, void* listener, PFN_on_event on_event);  // un register an event, takes in the same as above

// fires an event to listeners of the given code. if an event handler returns true, the event is considered handled and is not passed on to any more listeners
// @param code the evet code to fire
// @param sender a pointer to the sender, can be null
// @param data the event data
// @returns true if handled; otherwise false
KAPI b8 event_fire(u16 code, void* sender, event_context context);  // difference here is pointing to the sender, and along the data - any listeners registered for this code will recieve the data

// basic event codes that are only for within the engine
// system internal event codes. application should use codes beyond 255
typedef enum system_event_code {
    // shuts the application down on the next frame
    EVENT_CODE_APPLICATION_QUIT = 0x01,  // tells the application to quit

    // keylboard key pressed
    // Context usage:
    // u16 key_code = data.data.u16[0]
    EVENT_CODE_KEY_PRESSED = 0x02,

    // keylboard key released
    // Context usage:
    // u16 key_code = data.data.u16[0]
    EVENT_CODE_KEY_RELEASED = 0x03,

    // mouse button pressed
    // Context usage:
    // u16 button = data.data.u16[0]
    EVENT_CODE_BUTTON_PRESSED = 0x04,

    // mouse button released
    // Context usage:
    // u16 button = data.data.u16[0]
    EVENT_CODE_BUTTON_RELEASED = 0x05,

    // mouse moved
    // Context usage:
    // u16 x = data.data.u16[0]
    // u16 y = data.data.u16[1]
    EVENT_CODE_MOUSE_MOVED = 0x06,

    // mouse moved
    // Context usage:
    // u16 Z = data.data.u16[0]
    EVENT_CODE_MOUSE_WHEEL = 0x07,

    // resized/resolution changed from the os.
    // Context usage:
    // u16 width = data.data.u16[0]
    // u16 width = data.data.u16[1]
    EVENT_CODE_RESIZED = 0x08,

    // change the render mode for debugging purposes
    // context usage:
    // i32 mode = context.data.i32[0];
    EVENT_CODE_SET_RENDER_MODE = 0x0A,

    // here are some event codes for debugging
    EVENT_CODE_DEBUG0 = 0x10,
    EVENT_CODE_DEBUG1 = 0x11,
    EVENT_CODE_DEBUG2 = 0x12,
    EVENT_CODE_DEBUG3 = 0x13,
    EVENT_CODE_DEBUG4 = 0x14,

    MAX_EVENT_CODE = 0xFF
} system_event_code;
