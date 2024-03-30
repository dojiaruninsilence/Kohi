#pragma once

#include "defines.h"

// returns the length of the given string
KAPI u64 string_length(const char* str);

KAPI char* string_duplicate(const char* str);

// case sensitive string comparison. true if they are the same otherwise its false
KAPI b8 strings_equal(const char* str0, const char* str1);  // takes in pointers to 2 strings

// case insensitive string comparison. true if they are the same otherwise its false
KAPI b8 strings_equali(const char* str0, const char* str1);  // takes in pointers to 2 strings

// performs string formatting to dest given format string and parameters
// pass in a pointer to a destination that is large enough to hold the final string, the format to use, aand the dots represent any number of arguments to throw in
KAPI i32 string_format(char* dest, const char* format, ...);

// performs variadic string formatting to dest given format string and va_list
// @param dest the destination for the formatted string
// @param format the string to be formatted
// @param va_list the variadic argument list
// @ returns the size of the data written
KAPI i32 string_format_v(char* dest, const char* format, void* va_list);
