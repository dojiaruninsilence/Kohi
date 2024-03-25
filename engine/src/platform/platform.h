#pragma once

#include "defines.h"

// NOTE: these are all generic functions that will call different platform specific commands, but keep the functions uniform throughout the engine

// these are all for the environment the final application will be running on, and will be dependent on user input, system and such
b8 platform_system_startup(
    u64* memory_requirement,       // a pointer to where the memory requirement field is
    void* state,                   // a pointer to where the state is being held
    const char* application_name,  // mostly for windowed apps
    i32 x,
    i32 y,
    i32 width,
    i32 height);

// do all the shut down stuff - pass in a pointer to the platform state
void platform_system_shutdown(void* plat_state);

// needs to be done continuosly through out the run loop - will be coming back to this he says
b8 platform_pump_messages();

// this is simple in the beginning but becomes more complicated as we progress - so see this and pay close attention
// platform specific ways to handle memory - different platforms handle memory differently
void* platform_allocate(u64 size, b8 aligned);
void platform_free(void* block, b8 aligned);
void* platform_zero_memory(void* block, u64 size);
void* platform_copy_memory(void* dest, const void* source, u64 size);
void* platform_set_memory(void* dest, i32 value, u64 size);

// a way to write out color coded text to the console - is handled differently in different environments
void platform_console_write(const char* message, u8 colour);
void platform_console_write_error(const char* message, u8 colour);

// also need a way to retrieve the time in the platform layer - this is done differently one each platform
f64 platform_get_absolute_time();

// Sleep on the thread for the provided ms this blocks the main thread. ---miliseconds(ms)
// Should only be used for giving time back to the OS for unused uopdate power.
// therefore it is not exported.
void platform_sleep(u64 ms);