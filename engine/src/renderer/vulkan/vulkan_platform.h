#pragma once

#include "defines.h"

// foreward declarations
struct platform_state;
struct vulkan_context;

b8 platform_create_vulkan_surface(struct vulkan_context* context);  // vulkan needs a surface to render to, this will come from the windowing system, the os  pass in the vulkan context

// these will be defined in the respective platform.c files in the platform folder, like all other platform specific stuffs
// appends the names of required extensions for this platform to the names_darray, which should be created and passed in
void platform_get_required_extension_names(const char*** names_darray);  // need to figure out why there are 3 pointer symbols here - triple pointer(pointer to an array of strings(an array of chars))- pt to array then pt to string then pt to char, so 3X