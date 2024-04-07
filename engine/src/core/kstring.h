#pragma once

#include "defines.h"
#include "math/math_types.h"

// returns the length of the given string
KAPI u64 string_length(const char* str);

// returns a duplicate copy of the string that is input
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

// @brief Empties the provided string by setting the first character to 0
// @param str the string to be emptied
// @return a pointer to str
KAPI char* string_empty(char* str);

// copies a string from the source to the destination and returns a pointer to the destination
KAPI char* string_copy(char* dest, const char* source);

// copies a string from the source to the destination and returns a pointer to the destination - this one has a max length that can be copied
KAPI char* string_ncopy(char* dest, const char* source, i64 length);

// trim the white space off of both sides of a string
KAPI char* string_trim(char* str);

// copy a substring of a string, copy from the destination from the start point for the amount of length into the destination
KAPI void string_mid(char* dest, const char* source, i32 start, i32 length);

// @brief returns the index of the first occurance of c in str, otherwise -1
// @param str the string to be scanned
// @param c the character to search for
// @return the index of the first occurance of c, otherwise -1 if not found
KAPI i32 string_index_of(char* str, char c);

// @brief attempts to parse a vector from the provided string
// @param str the string to parse from. should be space-delimited. (i.e. "1.0 2.0 3.0 4.0")
// @param out_vector a pointer to the vector to write to
// @return true if parsed successfully, otherwise false
KAPI b8 string_to_vec4(char* str, vec4* out_vector);

// @brief attempts to parse a vector from the provided string
// @param str the string to parse from. should be space-delimited. (i.e. "1.0 2.0 3.0")
// @param out_vector a pointer to the vector to write to
// @return true if parsed successfully, otherwise false
KAPI b8 string_to_vec3(char* str, vec3* out_vector);

// @brief attempts to parse a vector from the provided string
// @param str the string to parse from. should be space-delimited. (i.e. "1.0 2.0")
// @param out_vector a pointer to the vector to write to
// @return true if parsed successfully, otherwise false
KAPI b8 string_to_vec2(char* str, vec2* out_vector);

// @brief attempts to parse a 32 bit floating point number from the provided string
// @param str the strin to parse from. should NOT be postfixed with  'f'
// @param f a pointer to the float to write to
// true if parsed successfully, false otherwise
KAPI b8 string_to_f32(char* str, f32* f);

// @brief attempts to parse a 64 bit floating point number from the provided string
// @param str the strin to parse from.
// @param f a pointer to the float to write to
// true if parsed successfully, false otherwise
KAPI b8 string_to_f64(char* str, f64* f);

// @brief attempts to parse a 8 bit signed integer from the provided string
// @param str the strin to parse from.
// @param i a pointer to the int to write to
// true if parsed successfully, false otherwise
KAPI b8 string_to_i8(char* str, i8* i);

// @brief attempts to parse a 16 bit signed integer from the provided string
// @param str the strin to parse from.
// @param i a pointer to the int to write to
// true if parsed successfully, false otherwise
KAPI b8 string_to_i16(char* str, i16* i);

// @brief attempts to parse a 32 bit signed integer from the provided string
// @param str the strin to parse from.
// @param i a pointer to the int to write to
// true if parsed successfully, false otherwise
KAPI b8 string_to_i32(char* str, i32* i);

// @brief attempts to parse a 64 bit signed integer from the provided string
// @param str the strin to parse from.
// @param i a pointer to the int to write to
// true if parsed successfully, false otherwise
KAPI b8 string_to_i64(char* str, i64* i);

// @brief attempts to parse a 8 bit unsigned integer from the provided string
// @param str the strin to parse from.
// @param u a pointer to the int to write to
// true if parsed successfully, false otherwise
KAPI b8 string_to_u8(char* str, u8* u);

// @brief attempts to parse a 16 bit unsigned integer from the provided string
// @param str the strin to parse from.
// @param u a pointer to the int to write to
// true if parsed successfully, false otherwise
KAPI b8 string_to_u16(char* str, u16* u);

// @brief attempts to parse a 32 bit unsigned integer from the provided string
// @param str the strin to parse from.
// @param u a pointer to the int to write to
// true if parsed successfully, false otherwise
KAPI b8 string_to_u32(char* str, u32* u);

// @brief attempts to parse a 64 bit unsigned integer from the provided string
// @param str the strin to parse from.
// @param u a pointer to the int to write to
// true if parsed successfully, false otherwise
KAPI b8 string_to_u64(char* str, u64* u);

// @brief attempts to parse a boolean from the provided string
// "true" or "1" are considered true, everthing else is false
// @param str the string to parse from.
// @param b a pointer to the int to write to
// true if parsed successfully, false otherwise
KAPI b8 string_to_b8(char* str, b8* b);