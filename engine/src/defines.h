#pragma once

// Unsigned int types.  -  instead of typing whole thing out type how many bytes it takes - u for unsigned
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

// Signed int types.  -  instead of typing whole thing out type how many bytes it takes - i for signed
typedef signed char i8;
typedef signed short i16;
typedef signed int i32;
typedef signed long long i64;

// Floating point types  -  instead of typing whole thing out type how many bytes it takes - f for floating types
typedef float f32;
typedef double f64;

// Boolean types    -  instead of typing whole thing out type how many bytes it takes - b for boolean types
typedef int b32;
typedef char b8;

// Properly define static assertions.
#if defined(__clang__) || defined(__gcc__)
#define STATIC_ASSERT _Static_assert
#else
#define STATIC_ASSERT static_assert
#endif

// Ensure all types are of the correct size. -- need to look this up if it doesnt get explained
STATIC_ASSERT(sizeof(u8) == 1, "Expected u8 to be 1 byte.");
STATIC_ASSERT(sizeof(u16) == 2, "Expected u16 to be 2 bytes.");
STATIC_ASSERT(sizeof(u32) == 4, "Expected u32 to be 4 bytes.");
STATIC_ASSERT(sizeof(u64) == 8, "Expected u64 to be 8 bytes.");

STATIC_ASSERT(sizeof(i8) == 1, "Expected i8 to be 1 byte.");
STATIC_ASSERT(sizeof(i16) == 2, "Expected i16 to be 2 bytes.");
STATIC_ASSERT(sizeof(i32) == 4, "Expected i32 to be 4 bytes.");
STATIC_ASSERT(sizeof(i64) == 8, "Expected i64 to be 8 bytes.");

STATIC_ASSERT(sizeof(f32) == 4, "Expected f32 to be 4 bytes.");
STATIC_ASSERT(sizeof(f64) == 8, "Expected f64 to be 8 bytes.");

#define TRUE 1
#define FALSE 0

// Platform detection windows first --- need to learn more about the pre process things - look this up
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
#define KPLATFORM_WINDOWS 1
#ifndef _WIN64
#error "64-bit is required on Windows!"
#endif
#elif defined(__linux__) || defined(__gnu_linux__)
// Linux OS
#define KPLATFORM_LINUX 1
#if defined(__ANDROID__)
#define KPLATFORM_ANDROID 1
#endif
#elif defined(__unix__)
// Catch anything not caught by the above.
#define KPLATFORM_UNIX 1
#elif defined(_POSIX_VERSION)
// Posix
#define KPLATFORM_POSIX 1
#elif __APPLE__
// Apple platforms
#define KPLATFORM_APPLE 1
#include <TargetConditionals.h>
#if TARGET_IPHONE_SIMULATOR
// iOS Simulator
#define KPLATFORM_IOS 1
#define KPLATFORM_IOS_SIMULATOR 1
#elif TARGET_OS_IPHONE
#define KPLATFORM_IOS 1
// iOS device
#elif TARGET_OS_MAC
// Other kinds of Mac OS
#else
#error "Unknown Apple platform"
#endif
#else
#error "Unknown platform!"
#endif

#ifdef KEXPORT
// Exports
#ifdef _MSC_VER
#define KAPI __declspec(dllexport)
#else
#define KAPI __attribute__((visibility("default")))
#endif
#else
// Imports
#ifdef _MSC_VER
#define KAPI __declspec(dllimport)
#else
#define KAPI
#endif
#endif

// if value is above the max, then value becomes max, or if value is less than min, then value becomes min
#define KCLAMP(value, min, max) (value <= min) ? min : (value >= max) ? max \
                                                                      : value;

// inlining
#ifdef _MSC_VER
#define KINLINE __forceinline           // removes the fuction call and just runs the code inside, which can help speed things up
#define KNOINLINE __declspec(noinline)  // this one says do not do an inline for any reason
#else
#define KINLINE static inline  // removes the fuction call and just runs the code inside, which can help speed things up
#define KNOINLINE
#endif