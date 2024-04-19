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

// @brief case sensitive string comparison for a number of characters.
// @param str0 the first string to be compared
// @param str1 the second string to be compared
// @param length the maximum number of characters to be compared
// @return true if the same, otherwise false
KAPI b8 strings_nequal(const char* str0, const char* str1, u64 length);

// @brief case insensitive string comparison for a number of characters
// @param str0 the first string to be compared
// @param str1 the second string to be compared
// @param length the maximum number of characters to be compared
// @return true if the same, otherwise false
KAPI b8 strings_nequali(const char* str0, const char* str1, u64 length);

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
KAPI b8 string_to_bool(char* str, b8* b);

// @brief splits the given string by the delimiter provided and stores in the provided darray.  optionally trims each array.
// NOTE: a string allocation occurs for each entry and must be freed by the caller
// @param str the string to be split
// @param delimiter the character to split by
// @param str_darray a pointer to a darray of char arrays to hold the entries NOTE: must be a darray
// @param trim_entries trims each entry if true
// @param include_empty indicates if empty entries should be included
// @return the number of entries yielded by the split operation
KAPI u32 string_split(const char* str, char delimeter, char*** str_darray, b8 trim_entries, b8 include_empty);

// @brief cleans up string allocations in str_darray, but does not free the darray itself
// @param str_darray the darray to be cleaned up
KAPI void string_cleanup_split_array(char** str_darray);

// @brief appends append to source and returns a new string
// @param dest the destination string
// @param source the string to be appended to
// @param append the string to append to source
// @returns a new string containing the concatenation of the two strings
KAPI void string_append_string(char* dest, const char* source, const char* append);

// @brief appends the supplied integer to source and outputs to dest
// @brief dest the destination for the string
// @param source the string to be appended to
// @param i the interger to be appended
KAPI void string_append_int(char* dest, const char* source, i64 i);

// @brief appends the supplied float to source and outputs to dest
// @brief dest the destination for the string
// @param source the string to be appended to
// @param f the float to be appended
KAPI void string_append_float(char* dest, const char* source, f32 f);

// @brief appends the supplied boolean (as either "true" or "false") to source and outputs to dest
// @brief dest the destination for the string
// @param source the string to be appended to
// @param b the boolean to be appended
KAPI void string_append_bool(char* dest, const char* source, b8 b);

// @brief appends the supplied character to source and outputs to dest
// @brief dest the destination for the string
// @param source the string to be appended to
// @param c the character to be appended
KAPI void string_append_char(char* dest, const char* source, char c);

// @brief extracts the directory from a full file path
// @param dest the destination for the path
// @param path the full path to extract from
KAPI void string_directory_from_path(char* dest, const char* path);

// @brief extracts the filename (including file extension) from a full file path
// @param dest the destination for the filename
// @param path the full path to extract from
KAPI void string_filename_from_path(char* dest, const char* path);

// @brief extracts the filename (excluding file extension) from a full file path
// @param dest the destination for the filename
// @param path the full path to extract from
KAPI void string_filename_no_extension_from_path(char* dest, const char* path);