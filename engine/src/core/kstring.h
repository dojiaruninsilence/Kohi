#pragma once

#include "defines.h"

// returns the length of the given string
KAPI u64 string_length(const char* str);

KAPI char* string_duplicate(const char* str);

// case sensitive string comparison. true if they are the same otherwise its false
KAPI b8 strings_equal(const char* str0, const char* str1);  // takes in pointers to 2 strings
