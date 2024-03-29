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

// define a pointer to the input syste state -- all we are storing on the stack for now is a pointer to the allocation
static input_state* state_ptr;

// initialize the input subsystem, - always call twice - on first pass pass in the memory requirement to get the memory required, and zero for the state
// on the second pass - pass in the state as well as the memory rewuirement and actually initialize the subsystem
void input_system_initialize(u64* memory_requirement, void* state) {
    *memory_requirement = sizeof(input_state);  // dereference the memory requirement and input the size of the input system state
    if (state == 0) {                           // if no state was passed in
        return;                                 // boot out
    }
    kzero_memory(state, sizeof(input_state));  // to be sure zero out the memory  pass in the size of the state and a pointer to it
    state_ptr = state;                         // pass the pointer throgh to state ptr
    KINFO("Input subsystem initialized");
}

void input_system_shutdown(void* state) {
    // TODO: add shutdown routines when needed -- some of the inputs on other devices are much more complex
    state_ptr = 0;  // all we are doing for now is reseting the state ptr
}

// this is called every frame
void input_update(f64 delta_time) {
    if (!state_ptr) {
        return;
    }

    // copy current states to previous states
    kcopy_memory(&state_ptr->keyboard_previous, &state_ptr->keyboard_current, sizeof(keyboard_state));
    kcopy_memory(&state_ptr->mouse_previous, &state_ptr->mouse_current, sizeof(mouse_state));
}

// keyboard internal functions
void input_process_key(keys key, b8 pressed) {  // takes in a key and whether it is pressed or not
    // only handle this if the state has actually changed
    if (state_ptr && state_ptr->keyboard_current.keys[key] != pressed) {  // check to see if the state has actually changed
        // update internal state
        state_ptr->keyboard_current.keys[key] = pressed;  // so if they arent equal then set them to equal

        // just a check to see if left and right keys are working
        if (key == KEY_LALT) {
            KINFO("Left alt %s.", pressed ? "pressed" : "released");
        } else if (key == KEY_RALT) {
            KINFO("Right alt %s.", pressed ? "pressed" : "released");
        }

        if (key == KEY_LCONTROL) {
            KINFO("Left ctrl %s.", pressed ? "pressed" : "released");
        } else if (key == KEY_RCONTROL) {
            KINFO("Right ctrl %s.", pressed ? "pressed" : "released");
        }

        if (key == KEY_LSHIFT) {
            KINFO("Left shift %s.", pressed ? "pressed" : "released");
        } else if (key == KEY_RSHIFT) {
            KINFO("Right shift %s.", pressed ? "pressed" : "released");
        }

        // fire off an event for immediate processing
        event_context context;                                                               // create event context
        context.data.u16[0] = key;                                                           // set the u16 to key code
        event_fire(pressed ? EVENT_CODE_KEY_PRESSED : EVENT_CODE_KEY_RELEASED, 0, context);  // if it is pressed(true) (?) then fire off KEY_PRESSED (:) if not then fire off KEY_RELEASED , there is no instance to send so set to 0, pass in the context, which i believe contains the keycode
    }
}

// mouse internal functions
void input_process_button(buttons button, b8 pressed) {
    // if the state has changed, fire an event
    if (state_ptr->mouse_current.buttons[button] != pressed) {  // check to see if the state has actually changed
        state_ptr->mouse_current.buttons[button] = pressed;     // if they have changed set current button to pressed

        // fire the event
        event_context context;                                                                     // create event context
        context.data.u16[0] = button;                                                              // set the u16 to the button code
        event_fire(pressed ? EVENT_CODE_BUTTON_PRESSED : EVENT_CODE_BUTTON_RELEASED, 0, context);  // if it is pressed(true) (?) then fire off BUTTON_PRESSED (:) if not then fire off BUTTON_RELEASED , there is no instance to send so set to 0, pass in the context, which i believe contains the keycode
    }
}

void input_process_mouse_move(i16 x, i16 y) {
    // only process if actually different
    if (state_ptr->mouse_current.x != x || state_ptr->mouse_current.y != y) {  // check to see if either of the positions have changed
        // NOTE: enable this if debugging.
        // KDEBUG("Mouse pos: %i, %i!", x, y);

        // update internal state
        state_ptr->mouse_current.x = x;  // if these have changed then set current to the new values
        state_ptr->mouse_current.y = y;

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
    if (!state_ptr) {  // if not initialized wont have a valid state
        return false;
    }
    return state_ptr->keyboard_current.keys[key] == true;
}

b8 input_is_key_up(keys key) {
    if (!state_ptr) {  // if not initialized wont have a valid state
        return true;
    }
    return state_ptr->keyboard_current.keys[key] == false;
}

b8 input_was_key_down(keys key) {
    if (!state_ptr) {  // if not initialized wont have a valid state
        return false;
    }
    return state_ptr->keyboard_previous.keys[key] == true;
}

b8 input_was_key_up(keys key) {
    if (!state_ptr) {  // if not initialized wont have a valid state
        return true;
    }
    return state_ptr->keyboard_previous.keys[key] == false;
}

// mouse user availabel functions
b8 input_is_button_down(buttons button) {
    if (!state_ptr) {
        return false;
    }
    return state_ptr->mouse_current.buttons[button] == true;
}

b8 input_is_button_up(buttons button) {
    if (!state_ptr) {
        return true;
    }
    return state_ptr->mouse_current.buttons[button] == false;
}

b8 input_was_button_down(buttons button) {
    if (!state_ptr) {
        return false;
    }
    return state_ptr->mouse_previous.buttons[button] == true;
}

b8 input_was_button_up(buttons button) {
    if (!state_ptr) {
        return true;
    }
    return state_ptr->mouse_previous.buttons[button] == false;
}

void input_get_mouse_position(i32* x, i32* y) {
    if (!state_ptr) {
        *x = 0;
        *y = 0;
        return;
    }
    *x = state_ptr->mouse_current.x;
    *y = state_ptr->mouse_current.y;
}

void input_get_previous_mouse_position(i32* x, i32* y) {
    if (!state_ptr) {
        *x = 0;
        *y = 0;
        return;
    }
    *x = state_ptr->mouse_previous.x;
    *y = state_ptr->mouse_previous.y;
}