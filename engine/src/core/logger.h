#pragma once

#include "defines.h"

// switches to disable the logging if needed, fatal and error will always be active, even in release builds
#define LOG_WARN_ENABLED 1
#define LOG_INFO_ENABLED 1
#define LOG_DEBUG_ENABLED 1
#define LOG_TRACE_ENABLED 1

// disables debug and trace logging in release builds
#if KRELEASE == 1
#define LOG_DEBUG_ENABLED 0
#define LOG_TRACE_ENABLED 0
#endif

typedef enum log_level {
    LOG_LEVEL_FATAL = 0,  // application cannot run and has to crash
    LOG_LEVEL_ERROR = 1,  // application will not run correctly, may crash, may recover
    LOG_LEVEL_WARN = 2,   // application is running sub optimal, but still running
    LOG_LEVEL_INFO = 3,   // logging for info only
    LOG_LEVEL_DEBUG = 4,  // same as info but only runs in debug builds
    LOG_LEVEL_TRACE = 5   // same as debug but for more verbose statements -- use sparingly
} log_level;

// do various things to stand system up - pass a pointer to a u64 to store the size of memory needed to store the state, and a pointer to where the state info will be stored
// initialize the logging system - call twice, once with state zeroed to ge required memory size, then a second time passind allocated memory to state -- returns true on success, false if failed
b8 initilize_logging(u64* memory_requirement, void* state);  // create files and such coming back to

// shutdown the loggin system
void shutdown_logging(void* state);

// logger system output - takes in the log level(look above) , and message, and then a list of arguments
KAPI void log_output(log_level level, const char* message, ...);  // need to look up, doesnt make a ton of sense.  gonna be where the logging funnels through

// logs a fatal-level message
#define KFATAL(message, ...) log_output(LOG_LEVEL_FATAL, message, ##__VA_ARGS__);  // need to look up, doesnt make a ton of sense.  gonna be where the logging funnels through -- has to do with clang compiling

#ifndef KERROR
// logs an error level message
#define KERROR(message, ...) log_output(LOG_LEVEL_ERROR, message, ##__VA_ARGS__);  // same as last but wrapped in if statements
#endif

#if LOG_WARN_ENABLED == 1
// logs an warning level message
#define KWARN(message, ...) log_output(LOG_LEVEL_WARN, message, ##__VA_ARGS__);  // same as last
#else
// does nothing if log warn is not enabled
#define KWARN(message, ...)
#endif

#if LOG_INFO_ENABLED == 1
// logs an info level message
#define KINFO(message, ...) log_output(LOG_LEVEL_INFO, message, ##__VA_ARGS__);  // same as last
#else
// does nothing if log info is not enabled
#define KINFO(message, ...)
#endif

#if LOG_DEBUG_ENABLED == 1
// logs an debug level message
#define KDEBUG(message, ...) log_output(LOG_LEVEL_DEBUG, message, ##__VA_ARGS__);  // same as last
#else
// does nothing if log debug is not enabled
#define KDEBUG(message, ...)
#endif

#if LOG_TRACE_ENABLED == 1
// logs an trace level message
#define KTRACE(message, ...) log_output(LOG_LEVEL_TRACE, message, ##__VA_ARGS__);  // same as last
#else
// does nothing if log trace is not enabled
#define KTRACE(message, ...)
#endif
