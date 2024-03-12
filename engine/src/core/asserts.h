#pragma once

#include "defines.h"

// disable assertions by commenting out the below line
#define KASSERTIONS_ENABLED

// helps with different compilers and such. need to check this out more
#ifdef KASSERTIONS_ENABLED
#if _MSC_VER
#include <intrin.h>
#define debugBreak() __debugbreak()
#else
#define debugBreak() __builtin_trap()
#endif

KAPI void report_assertion_failure(const char* expression, const char* message, const char* file, i32 line);  // method to output info to us to see what happened, a string that can be output a message where needed, the file the error was and the line

// so when you add an assert it will evaluate the expression input and if its true it does nothing, if it is false it asserts
// out put the expression, file, and line number
#define KASSERT(expr)                                                \
    {                                                                \
        if (expr) {                                                  \
        } else {                                                     \
            report_assertion_failure(#expr, "", __FILE__, __LINE__); \
            debugBreak();                                            \
        }                                                            \
    }
// out put the expression, message, file, and line number
#define KASSERT_MSG(expr, message)                                        \
    {                                                                     \
        if (expr) {                                                       \
        } else {                                                          \
            report_assertion_failure(#expr, message, __FILE__, __LINE__); \
            debugBreak();                                                 \
        }                                                                 \
    }

// debug asserts -- only in debug mode
// out put the expression, file, and line number
#ifdef _DEBUG
#define KASSERT_DEBUG(expr)                                          \
    {                                                                \
        if (expr) {                                                  \
        } else {                                                     \
            report_assertion_failure(#expr, "", __FILE__, __LINE__); \
            debugBreak();                                            \
        }                                                            \
    }

// out put the expression, message, file, and line number
#define KASSERT_DEBUG_MSG(expr, message)                                  \
    {                                                                     \
        if (expr) {                                                       \
        } else {                                                          \
            report_assertion_failure(#expr, message, __FILE__, __LINE__); \
            debugBreak();                                                 \
        }                                                                 \
    }
#else
#define KASSERT_DEBUG(expr)      // does nothing
#define KASSERT_DEBUG_MSG(expr)  // does nothing
#endif

// if asserts are disabled, call all the functions but do nothing

#else
#define KASSERT(expr)
#define KASSERT_MSG(expr, message)
#define KASSERT_DEBUG(expr)
#define KASSERT_DEBUG_MSG(expr, message)
#endif