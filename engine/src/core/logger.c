#include "logger.h"
#include "asserts.h"
#include "platform/platform.h"

// TODO: temporary - will be removed
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

// creating a simple logging system now will evolve as the project grows

// this is just a dummy logging state, will come back and correct
typedef struct logger_system_state {
    b8 initialized;
} logger_system_state;

// holds a private pointer to the logger system state - this willbe the only part of the state stored on the stack
static logger_system_state* state_ptr;

// initialize the logging system - pass a pointer to a u64 to store the size of memory needed to store the state, and a pointer to where the state info will be stored
b8 initilize_logging(u64* memory_requirement, void* state) {
    *memory_requirement = sizeof(logger_system_state);  // dereference memory requirement and set it equal to the size of the logger system state, - this always happens, this is required
    if (state == 0) {                                   // if no pointer to a state is passed in
        return true;                                    // stop here and boot out ruturning true
    }

    state_ptr = state;              // pass through the pointer
    state_ptr->initialized = true;  // set the state to initialized

        // test stuff TODO: will be removed
    KFATAL("a test message: %f", 3.14f);
    KERROR("a test message: %f", 3.14f);
    KWARN("a test message: %f", 3.14f);
    KINFO("a test message: %f", 3.14f);
    KDEBUG("a test message: %f", 3.14f);
    KTRACE("a test message: %f", 3.14f);

    // TODO: create log file.
    return true;
}

// shut down the logging system
void shutdown_logging(void* state) {
    // TODO: cleanup logging/write queued entries
    state_ptr = 0;  // all we are doing for now is resetting the state pointer
}

// sogging system output
void log_output(log_level level, const char* message, ...) {
    const char* level_strings[6] = {"[FATAL]: ", "[ERROR]: ", "[WARN]:  ", "[INFO]:  ", "[DEBUG]: ", "[TRACE]: "};
    b8 is_error = level < LOG_LEVEL_WARN;  // is it error level or higher

    // this will technically allow a 32k character limit, but dont do it
    // this is allocating a spot on the stack, this is for performance, faster than using the heap, look into this further.  to avoid dynamic allocation
    const i32 msg_length = 32000;
    char out_message[msg_length];
    memset(out_message, 0, sizeof(out_message));

    // format the original message in a string
    // NOTE: ms headers override the gcc/clang va_list type with a "typedef char * va_list" in some cases and as a result throws strange error here.
    // the workaround he uses is the __builtin_va_list, which is the type that gcc/clang expects
    __builtin_va_list arg_ptr;                             // creates a char array pointer to the ... list
    va_start(arg_ptr, message);                            // start usins list, first arg is message
    vsnprintf(out_message, msg_length, message, arg_ptr);  // takes message, a size of buffer and outputs the string
    va_end(arg_ptr);                                       // cleans everything up

    char out_message2[msg_length];
    sprintf(out_message2, "%s%s\n", level_strings[level], out_message);  // prepends the level to the message string FATAL, ERROR, ect. out message is both the buffer and the output

    // platform specific output. - takes in a message and the level of the message - and outputs per operating system
    if (is_error) {  // if error use the error stream if possible
        platform_console_write_error(out_message2, level);
    } else {
        platform_console_write(out_message2, level);
    }
}

// declaration from asserts.h -- sends assert to log with fatal level and info from assert
void report_assertion_failure(const char* expression, const char* message, const char* file, i32 line) {
    log_output(LOG_LEVEL_FATAL, "Assertion Faillure: %s, message: '%s', in file: %s, line: %d\n", expression, message, file, line);
}