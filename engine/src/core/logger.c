#include "logger.h"
#include "asserts.h"
#include "platform/platform.h"
#include "platform/filesystem.h"
#include "core/kstring.h"
#include "core/kmemory.h"

// TODO: temporary - will be removed
#include <stdarg.h>

// creating a simple logging system now will evolve as the project grows

// this is just a dummy logging state, will come back and correct
typedef struct logger_system_state {
    file_handle log_file_handle;  // used for creating a log file of events
} logger_system_state;

// holds a private pointer to the logger system state - this willbe the only part of the state stored on the stack
static logger_system_state* state_ptr;

// a way to append to the log file, just pass in a message
void append_to_log_file(const char* message) {
    if (state_ptr && state_ptr->log_file_handle.is_valid) {
        // since the message already contains a '\n', just write the bytes directly
        u64 length = string_length(message);  // use string length to get the length of the message
        u64 written = 0;                      // reset written to 0
        // run the function to write to a file, pass in the handle to the log file, the length of the message, the message, and the place to hold the message
        if (!filesystem_write(&state_ptr->log_file_handle, length, message, &written)) {     // if it fails
            platform_console_write_error("ERROR writing to console.log.", LOG_LEVEL_ERROR);  // send an error to the console
        }
    }
}

// initialize the logging system - pass a pointer to a u64 to store the size of memory needed to store the state, and a pointer to where the state info will be stored
b8 initilize_logging(u64* memory_requirement, void* state) {
    *memory_requirement = sizeof(logger_system_state);  // dereference memory requirement and set it equal to the size of the logger system state, - this always happens, this is required
    if (state == 0) {                                   // if no pointer to a state is passed in
        return true;                                    // stop here and boot out ruturning true
    }

    state_ptr = state;  // pass through the pointer

    // create new/wipe existing log file, then open it. - the path is console.log, it is set to write mode, dont write in binary, and a pointer to the log file handle, where it will be held
    if (!filesystem_open("console.log", FILE_MODE_WRITE, false, &state_ptr->log_file_handle)) {           // if it fails
        platform_console_write_error("ERROR: unable to open console.log for writing.", LOG_LEVEL_ERROR);  // write an error to the console
        return false;                                                                                     // and boot out
    }

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
    // TODO: These string operations are all pretty slow. this needs to be moved to another thread eventually,
    // along with the file writes, to avoid slowing things down while the engine is trying to run
    const char* level_strings[6] = {"[FATAL]: ", "[ERROR]: ", "[WARN]:  ", "[INFO]:  ", "[DEBUG]: ", "[TRACE]: "};
    b8 is_error = level < 2;  // is it error level or higher

    // this will technically allow a 32k character limit, but dont do it
    // this is allocating a spot on the stack, this is for performance, faster than using the heap, look into this further.  to avoid dynamic allocation
    char out_message[32000];
    kzero_memory(out_message, sizeof(out_message));

    // format the original message in a string
    // NOTE: ms headers override the gcc/clang va_list type with a "typedef char * va_list" in some cases and as a result throws strange error here.
    // the workaround he uses is the __builtin_va_list, which is the type that gcc/clang expects
    __builtin_va_list arg_ptr;                       // creates a char array pointer to the ... list
    va_start(arg_ptr, message);                      // start usins list, first arg is message
    string_format_v(out_message, message, arg_ptr);  // our string format function, pass in a place to put the message, the format message, and a list of arguments
    va_end(arg_ptr);                                 // cleans everything up

    // prepends the level to the message string FATAL, ERROR, ect. out message is both the buffer and the output
    string_format(out_message, "%s%s\n", level_strings[level], out_message);

    // platform specific output. - takes in a message and the level of the message - and outputs per operating system
    if (is_error) {  // if error use the error stream if possible
        platform_console_write_error(out_message, level);
    } else {
        platform_console_write(out_message, level);
    }

    // queue a copy to be written to the log file
    append_to_log_file(out_message);  // call private function to append the message to the log file
}

// declaration from asserts.h -- sends assert to log with fatal level and info from assert
void report_assertion_failure(const char* expression, const char* message, const char* file, i32 line) {
    log_output(LOG_LEVEL_FATAL, "Assertion Faillure: %s, message: '%s', in file: %s, line: %d\n", expression, message, file, line);
}