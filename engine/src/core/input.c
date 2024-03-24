#include "core/input.h"
#include "core/event.h"
#include "core/kmemory.h"
#include "core/logger.h"

typedef struct keyboard_state {
    b8 keys[256];  // match to defined keys
} keyboard_state;

typedef struct mouse_state {
    i16 x;
    i16 y;
    u8 buttons[BUTTON_MAX_BUTTONS];
} mouse_state;

// rather than having two copies of each struct to hold the current and previous states, add one that can control both
typedef struct input_state {
    keyboard_state keyboard_current;
    keyboard_state keyboard_previous;
    mouse_state mouse_current;
    mouse_state mouse_previous;
} input_state;

// internal input state
static b8 initialized = false;  // keep track of if it has been initialized
static input_state state = {};

void input_initialize() {
    kzero_memory(&state, sizeof(input_state));  // just going extra here
    initialized = true;
    KINFO("Input subsystem initialized");
}

void input_shutdown() {
    // TODO: add shutdown routines when needed -- some of the inputs on other devices are much more complex
    initialized = false;  // left off here the video is at 15 32
}

// this is called every frame
void input_update(f64 delta_time) {
    if (!initialized) {
        return;
    }

    // copy current states to previous states
    kcopy_memory(&state.keyboard_previous, &state.keyboard_current, sizeof(keyboard_state));
    kcopy_memory(&state.mouse_previous, &state.mouse_current, sizeof(mouse_state));
}

// keyboard internal functions
void input_process_key(keys key, b8 pressed) {  // takes in a key and whether it is pressed or not

    // just a check to see if left and right keys are working
    if (key == KEY_LALT) {
        KINFO("left alt key is pressed");
    } else if (key == KEY_RALT) {
        KINFO("right alt key is pressed");
    } else if (key == KEY_LSHIFT) {
        KINFO("left shift key is pressed");
    } else if (key == KEY_RSHIFT) {
        KINFO("right shift key is pressed");
    } else if (key == KEY_LCONTROL) {
        KINFO("left control key is pressed");
    } else if (key == KEY_RCONTROL) {
        KINFO("right control key is pressed");
    }

    // only handle this if the state has actually changed
    if (state.keyboard_current.keys[key] != pressed) {  // check to see if the state has actually changed
        // update internal state
        state.keyboard_current.keys[key] = pressed;  // so if they arent equal then set them to equal

        // fire off an event for immediate processing
        event_context context;                                                               // create event context
        context.data.u16[0] = key;                                                           // set the u16 to key code
        event_fire(pressed ? EVENT_CODE_KEY_PRESSED : EVENT_CODE_KEY_RELEASED, 0, context);  // if it is pressed(true) (?) then fire off KEY_PRESSED (:) if not then fire off KEY_RELEASED , there is no instance to send so set to 0, pass in the context, which i believe contains the keycode
    }
}

// mouse internal functions
void input_process_button(buttons button, b8 pressed) {
    // if the state has changed, fire an event
    if (state.mouse_current.buttons[button] != pressed) {  // check to see if the state has actually changed
        state.mouse_current.buttons[button] = pressed;     // if they have changed set current button to pressed

        // fire the event
        event_context context;                                                                     // create event context
        context.data.u16[0] = button;                                                              // set the u16 to the button code
        event_fire(pressed ? EVENT_CODE_BUTTON_PRESSED : EVENT_CODE_BUTTON_RELEASED, 0, context);  // if it is pressed(true) (?) then fire off BUTTON_PRESSED (:) if not then fire off BUTTON_RELEASED , there is no instance to send so set to 0, pass in the context, which i believe contains the keycode
    }
}

void input_process_mouse_move(i16 x, i16 y) {
    // only process if actually different
    if (state.mouse_current.x != x || state.mouse_current.y != y) {  // check to see if either of the positions have changed
        // NOTE: enable this if debugging.
        // KDEBUG("Mouse pos: %i, %i!", x, y);

        // update internal state
        state.mouse_current.x = x;  // if these have changed then set current to the new values
        state.mouse_current.y = y;

        // fire the event
        event_context context;    // create the context
        context.data.u16[0] = x;  // set the x and y values into the context
        context.data.u16[1] = y;
        event_fire(EVENT_CODE_MOUSE_MOVED, 0, context);
    }
}

void input_process_mouse_wheel(i8 z_delta) {
    // NOTE: no internal state to update

    // fire the event
    event_context context;
    context.data.u8[0] = z_delta;
    event_fire(EVENT_CODE_MOUSE_WHEEL, 0, context);
}

// keyboard user available functions --- these were all pretty straight foreward and i was tired so i didnt take many notes. this was in video #009 near the end of it 23 mins and before
b8 input_is_key_down(keys key) {
    if (!initialized) {  // if not initialized wont have a valid state
        return false;
    }
    return state.keyboard_current.keys[key] == true;
}

b8 input_is_key_up(keys key) {
    if (!initialized) {  // if not initialized wont have a valid state
        return true;
    }
    return state.keyboard_current.keys[key] == false;
}

b8 input_was_key_down(keys key) {
    if (!initialized) {  // if not initialized wont have a valid state
        return false;
    }
    return state.keyboard_previous.keys[key] == true;
}

b8 input_was_key_up(keys key) {
    if (!initialized) {  // if not initialized wont have a valid state
        return true;
    }
    return state.keyboard_previous.keys[key] == false;
}

// mouse user availabel functions
b8 input_is_button_down(buttons button) {
    if (!initialized) {
        return false;
    }
    return state.mouse_current.buttons[button] == true;
}

b8 input_is_button_up(buttons button) {
    if (!initialized) {
        return true;
    }
    return state.mouse_current.buttons[button] == false;
}

b8 input_was_button_down(buttons button) {
    if (!initialized) {
        return false;
    }
    return state.mouse_previous.buttons[button] == true;
}

b8 input_was_button_up(buttons button) {
    if (!initialized) {
        return true;
    }
    return state.mouse_previous.buttons[button] == false;
}

void input_get_mouse_position(i32* x, i32* y) {
    if (!initialized) {
        *x = 0;
        *y = 0;
        return;
    }
    *x = state.mouse_current.x;
    *y = state.mouse_current.y;
}

void input_get_previous_mouse_position(i32* x, i32* y) {
    if (!initialized) {
        *x = 0;
        *y = 0;
        return;
    }
    *x = state.mouse_previous.x;
    *y = state.mouse_previous.y;
}