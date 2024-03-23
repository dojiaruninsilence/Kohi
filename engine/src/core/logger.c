#include "logger.h"
#include "asserts.h"
#include "platform/platform.h"

// TODO: temporary - will be removed
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

// creating a simple logging system now will evolve as the project grows

b8 initilize_logging() {
    // TODO: create log file.
    return true;
}

void shutdown_logging() {
    // TODO: cleanup logging/write queued entries
}

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