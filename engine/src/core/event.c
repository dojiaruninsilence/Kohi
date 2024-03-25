#include "core/event.h"

#include "core/kmemory.h"
#include "containers/darray.h"

// structs to hold the needed data as it is beng handled by the event system
typedef struct registered_event {
    void* listener;
    PFN_on_event callback;
} registered_event;

// represents a single messaged code for registering multiple listeners
typedef struct event_code_entry {
    registered_event* events;
} event_code_entry;

// this should be more than enough codes....he says
#define MAX_MESSAGE_CODES 16384  // needed for the next struct

// state structure
typedef struct event_system_state {
    // lookup tables for event codes
    event_code_entry registered[MAX_MESSAGE_CODES];  // vreates an array with the amount of entries defined by max message codes define
} event_system_state;

// event system internal state pointer stuffs -- used to see if the event system is active and working
static event_system_state* state_ptr;

// initialize the event subsystem, - always call twice - on first pass pass in the memory requirement to get the memory required, and zero for the state
// on the second pass - pass in the state as well as the memory rewuirement and actually initialize the subsystem
void event_system_initialize(u64* memory_requirement, void* state) {
    *memory_requirement = sizeof(event_system_state);  // dereference the memory requirement and input the size of the event system state
    if (state == 0) {                                  // if no state was passed in
        return;                                        // boot out
    }
    kzero_memory(state, sizeof(state));  // if there was a state then zero out the memory using our function, passing in the state for both the address and the size
    state_ptr = state;                   // pass the pointer to state pointer, to track internally
}

void event_system_shutdown(void* state) {
    if (state_ptr) {
        // free the events arrays. and objects pointed to should be destroyed on their own
        for (u16 i = 0; i < MAX_MESSAGE_CODES; ++i) {             // interate through the registered events
            if (state_ptr->registered[i].events != 0) {           //  and if it has an array
                darray_destroy(state_ptr->registered[i].events);  // destroy the array
                state_ptr->registered[i].events = 0;              // goes through all the message codes and zeros anything in it out
            }
        }
    }
    state_ptr = 0;  // set the state pointer back to zero
}

b8 event_register(u16 code, void* listener, PFN_on_event on_event) {  // pass in the code, a pointer to the listener, and the function to pass when the event is fired
    if (!state_ptr) {                                                 // if a pointer to a state doesnt exist -- this where i left off - his code was line 52
        return false;
    }

    if (state_ptr->registered[code].events == 0) {                             // if there is nothing registered for this code
        state_ptr->registered[code].events = darray_create(registered_event);  // just create our dynamic array
    }

    u64 registered_count = darray_length(state_ptr->registered[code].events);
    for (u64 i = 0; i < registered_count; ++i) {
        if (state_ptr->registered[code].events[i].listener == listener) {  // check to see if the code has already been registered to prevent duplicates
            // TODO: warn
            return false;
        }
    }

    // if at this point there was no duplicate found. proceed with the registration
    registered_event event;
    event.listener = listener;                               // set the listener
    event.callback = on_event;                               // set the event
    darray_push(state_ptr->registered[code].events, event);  // push the info into the array

    return true;
}

b8 event_unregister(u16 code, void* listener, PFN_on_event on_event) {
    if (!state_ptr) {
        return false;
    }

    // on nothing is registered for the code then boot out
    if (state_ptr->registered[code].events == 0) {
        // TODO: warn
        return false;
    }

    u64 registered_count = darray_length(state_ptr->registered[code].events);
    for (u64 i = 0; i < registered_count; ++i) {
        registered_event e = state_ptr->registered[code].events[i];
        if (e.listener == listener && e.callback == on_event) {
            // found one now remove it
            registered_event popped_event;
            darray_pop_at(state_ptr->registered[code].events, i, &popped_event);
            return true;
        }
    }

    // not found
    return false;
}

b8 event_fire(u16 code, void* sender, event_context context) {
    if (!state_ptr) {
        return false;
    }

    // on nothing is registered for the code then boot out
    if (state_ptr->registered[code].events == 0) {
        // TODO: warn
        return false;
    }

    u64 registered_count = darray_length(state_ptr->registered[code].events);  // how many listeners are registered to this event
    for (u64 i = 0; i < registered_count; ++i) {                               // loop through the listeners
        registered_event e = state_ptr->registered[code].events[i];
        if (e.callback(code, sender, e.listener, context)) {
            // message has been handled do not send to other listeners
            return true;
        }
    }

    // not found
    return false;
}