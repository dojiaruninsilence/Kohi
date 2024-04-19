#include "core/kstring.h"
#include "core/kmemory.h"
#include "containers/darray.h"

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>  // isspace

#ifndef _MSC_VER
#include <strings.h>
#endif

u64 string_length(const char* str) {
    return strlen(str);
}

char* string_duplicate(const char* str) {
    u64 length = string_length(str);                        // get length of string to determine how much memory to reserve
    char* copy = kallocate(length + 1, MEMORY_TAG_STRING);  // allocate a new character array with lenght +1
    kcopy_memory(copy, str, length);
    copy[length] = 0;
    return copy;
}

// case sensitive string comparison. true if they are the same otherwise its false
b8 strings_equal(const char* str0, const char* str1) {
    return strcmp(str0, str1) == 0;  // gonna utilize the c library command to compare strings for now
}

// case insensitive string comparison. true if they are the same otherwise its false
b8 strings_equali(const char* str0, const char* str1) {
#if defined(__GNUC__)
    return strcasecmp(str0, str1) == 0;
#elif (defined _MSC_VER)
    return _strcmpi(str0, str1) == 0;
#endif
}

// @brief case sensitive string comparison for a number of characters.
// @param str0 the first string to be compared
// @param str1 the second string to be compared
// @param length the maximum number of characters to be compared
// @return true if the same, otherwise false
KAPI b8 strings_nequal(const char* str0, const char* str1, u64 length) {
    return strncmp(str0, str1, length);
}

// @brief case insensitive string comparison for a number of characters
// @param str0 the first string to be compared
// @param str1 the second string to be compared
// @param length the maximum number of characters to be compared
// @return true if the same, otherwise false
b8 strings_nequali(const char* str0, const char* str1, u64 length) {
#if defined(__GNUC__)
    return strncasecmp(str0, str1, length) == 0;
#elif (defined _MSC_VER)
    return _strnicmp(str0, str1, length) == 0;
#endif
}

// need to look upo these variadic arguments
// performs string formatting to dest given format string and parameters
// pass in a pointer to a destination that is large enough to hold the final string, the format to use, aand the dots represent any number of arguments to throw in
i32 string_format(char* dest, const char* format, ...) {
    if (dest) {                                                // if there is actually a place to put it
        __builtin_va_list arg_ptr;                             // create a variadic argument list ptr, im assuming this automattically pulls in all the arguments somehow and builds a list out of it
        va_start(arg_ptr, format);                             // start variadic argument, pass n the pointer and the format
        i32 written = string_format_v(dest, format, arg_ptr);  // pass the destination, format, and the variadic list to string format v and store the reuslt in written
        va_end(arg_ptr);                                       // end the variadic argument
        return written;                                        // return fromatted string
    }
    return -1;  // if there is no destination, return -1
}

// performs variadic string formatting to dest given format string and va_list
// @param dest the destination for the formatted string
// @param format the string to be formatted
// @param va_list the variadic argument list
// @ returns the size of the data written
i32 string_format_v(char* dest, const char* format, void* va_listp) {
    if (dest) {  // if there is actually a place to put it
        // big, but can fit on the stack
        char buffer[32000];                                        // create a char array buffer with a size of 32000, which will be the max string length
        i32 written = vsnprintf(buffer, 32000, format, va_listp);  // takes ina buffer to put the string, the max size, the format, and the variadic list to format and stores the length in written
        buffer[written] = 0;                                       // zeros out everything in buffer after the length of written
        kcopy_memory(dest, buffer, written + 1);                   // copies the new string stored in buffer to the destination, uses written plus one for the size

        return written;  // return the resulting string
    }
    return -1;  // if there is no destinstion, return -1
}

// @brief Empties the provided string by setting the first character to 0
// @param str the string to be emptied
// @return a pointer to str
char* string_empty(char* str) {
    if (str) {
        str[0] = 0;
    }

    return str;
}

// copies a string from the source to the destination and returns a pointer to the destination - just using the standard library functions for now
char* string_copy(char* dest, const char* source) {
    return strcpy(dest, source);
}

// copies a string from the source to the destination and returns a pointer to the destination - this one has a max length that can be copied - just using the standard library functions for now
char* string_ncopy(char* dest, const char* source, i64 length) {
    return strncpy(dest, source, length);
}

// trim the white space off of both sides of a string
char* string_trim(char* str) {
    while (isspace((unsigned char)*str)) {  // if the char is a space
        str++;                              // move the pointer passed the spaces
    }
    if (*str) {         // if the string still contains anything after moving passed the spaces
        char* p = str;  // take a copy of where the pointer is at
        while (*p) {    // while p has a value
            p++;        // move the pointer p until it reaches the end of the string
        }
        while (isspace((unsigned char)*(--p)))  // move the pointer from the end back passed any spaces at the end of the string
            ;
        p[1] = '\0';  // move the pointer one forward, to get on the first space at the end, and then trim the end off
    }

    return str;  // now with the spaces trimmed from the beggining and ending of the string
}

// copy a substring of a string, copy from the destination from the start point for the amount of length into the destination
void string_mid(char* dest, const char* source, i32 start, i32 length) {
    if (length == 0) {
        return;  // cant get a substring from nothing
    }
    u64 src_length = string_length(source);
    if (start >= src_length) {  // if the start point is after the end of the string
        dest[0] = 0;            // return 0
        return;
    }
    if (length > 0) {                                                    // an actual positive length was entered
        for (u64 i = start, j = 0; j < length && source[i]; ++i, ++j) {  // loop from the start point for the length input, and keep going until the length is reached, or the end of the source string is reached
            dest[j] = source[i];                                         // copy character from source to destination
        }
        dest[start + length] = 0;  // end the dest string
    } else {
        // if a negative value is passed proceed to the end of the string
        u64 j = 0;
        for (u64 i = start; source[i]; ++i, ++j) {  // loop through the string from the start point to the end of the string
            dest[j] = source[i];                    // copy character from source to destination
        }
        dest[start + j] = 0;  // end the dest string
    }
}

// @brief returns the index of the first occurance of c in str, otherwise -1
// @param str the string to be scanned
// @param c the character to search for
// @return the index of the first occurance of c, otherwise -1 if not found
i32 string_index_of(char* str, char c) {
    if (!str) {  // if no string was input
        return -1;
    }
    u32 length = string_length(str);        // get the length of the string
    if (length > 0) {                       // make sure that the string contains something
        for (u32 i = 0; i < length; ++i) {  // iterate through all of the chars in the string
            if (str[i] == c) {              // when you find the input character
                return i;                   // return its index
            }
        }
    }

    // if character was not found
    return -1;
}

// @brief attempts to parse a vector from the provided string
// @param str the string to parse from. should be space-delimited. (i.e. "1.0 2.0 3.0 4.0")
// @param out_vector a pointer to the vector to write to
// @return true if parsed successfully, otherwise false
b8 string_to_vec4(char* str, vec4* out_vector) {
    if (!str) {        // check that a string had been input
        return false;  // fail
    }

    kzero_memory(out_vector, sizeof(vec4));                                                                   // zero out the memory for the vec4
    i32 result = sscanf(str, "%f %f %f %f", &out_vector->x, &out_vector->y, &out_vector->z, &out_vector->w);  // look for the floats using the format string input to id where the values should be in the string
    return result != -1;                                                                                      // return the results as long as they are not -1
}

// @brief attempts to parse a vector from the provided string
// @param str the string to parse from. should be space-delimited. (i.e. "1.0 2.0 3.0")
// @param out_vector a pointer to the vector to write to
// @return true if parsed successfully, otherwise false
b8 string_to_vec3(char* str, vec3* out_vector) {
    if (!str) {        // check that a string had been input
        return false;  // fail
    }

    kzero_memory(out_vector, sizeof(vec3));                                                // zero out the memory for the vec3
    i32 result = sscanf(str, "%f %f %f", &out_vector->x, &out_vector->y, &out_vector->z);  // look for the floats using the format string input to id where the values should be in the string
    return result != -1;
}

// @brief attempts to parse a vector from the provided string
// @param str the string to parse from. should be space-delimited. (i.e. "1.0 2.0")
// @param out_vector a pointer to the vector to write to
// @return true if parsed successfully, otherwise false
b8 string_to_vec2(char* str, vec2* out_vector) {
    if (!str) {        // check that a string had been input
        return false;  // fail
    }

    kzero_memory(out_vector, sizeof(vec2));                             // zero out the memory for the vec2
    i32 result = sscanf(str, "%f %f", &out_vector->x, &out_vector->y);  // look for the floats using the format string input to id where the values should be in the string
    return result != -1;
}

// @brief attempts to parse a 32 bit floating point number from the provided string
// @param str the strin to parse from. should NOT be postfixed with  'f'
// @param f a pointer to the float to write to
// true if parsed successfully, false otherwise
b8 string_to_f32(char* str, f32* f) {
    if (!str) {        // check that a string had been input
        return false;  // fail
    }

    *f = 0;                             // zero out the memory for the f32
    i32 result = sscanf(str, "%f", f);  // look for the floats using the format string input to id where the values should be in the string
    return result != -1;
}

// @brief attempts to parse a 64 bit floating point number from the provided string
// @param str the strin to parse from.
// @param f a pointer to the float to write to
// true if parsed successfully, false otherwise
b8 string_to_f64(char* str, f64* f) {
    if (!str) {        // check that a string had been input
        return false;  // fail
    }

    *f = 0;                              // zero out the memory for the f32
    i32 result = sscanf(str, "%lf", f);  // look for the floats using the format string input to id where the values should be in the string
    return result != -1;
}

// @brief attempts to parse a 8 bit signed integer from the provided string
// @param str the strin to parse from.
// @param i a pointer to the int to write to
// true if parsed successfully, false otherwise
b8 string_to_i8(char* str, i8* i) {
    if (!str) {        // check that a string had been input
        return false;  // fail
    }

    *i = 0;                               // zero out the memory for the f32
    i32 result = sscanf(str, "%hhi", i);  // look for the floats using the format string input to id where the values should be in the string
    return result != -1;
}

// @brief attempts to parse a 16 bit signed integer from the provided string
// @param str the strin to parse from.
// @param i a pointer to the int to write to
// true if parsed successfully, false otherwise
b8 string_to_i16(char* str, i16* i) {
    if (!str) {        // check that a string had been input
        return false;  // fail
    }

    *i = 0;                              // zero out the memory for the f32
    i32 result = sscanf(str, "%hi", i);  // look for the floats using the format string input to id where the values should be in the string
    return result != -1;
}

// @brief attempts to parse a 32 bit signed integer from the provided string
// @param str the strin to parse from.
// @param i a pointer to the int to write to
// true if parsed successfully, false otherwise
b8 string_to_i32(char* str, i32* i) {
    if (!str) {        // check that a string had been input
        return false;  // fail
    }

    *i = 0;                             // zero out the memory for the f32
    i32 result = sscanf(str, "%i", i);  // look for the floats using the format string input to id where the values should be in the string
    return result != -1;
}

// @brief attempts to parse a 64 bit signed integer from the provided string
// @param str the strin to parse from.
// @param i a pointer to the int to write to
// true if parsed successfully, false otherwise
b8 string_to_i64(char* str, i64* i) {
    if (!str) {        // check that a string had been input
        return false;  // fail
    }

    *i = 0;                               // zero out the memory for the f32
    i32 result = sscanf(str, "%lli", i);  // look for the floats using the format string input to id where the values should be in the string
    return result != -1;
}

// @brief attempts to parse a 8 bit unsigned integer from the provided string
// @param str the strin to parse from.
// @param u a pointer to the int to write to
// true if parsed successfully, false otherwise
b8 string_to_u8(char* str, u8* u) {
    if (!str) {        // check that a string had been input
        return false;  // fail
    }

    *u = 0;                               // zero out the memory for the f32
    i32 result = sscanf(str, "%hhu", u);  // look for the floats using the format string input to id where the values should be in the string
    return result != -1;
}

// @brief attempts to parse a 16 bit unsigned integer from the provided string
// @param str the strin to parse from.
// @param u a pointer to the int to write to
// true if parsed successfully, false otherwise
b8 string_to_u16(char* str, u16* u) {
    if (!str) {        // check that a string had been input
        return false;  // fail
    }

    *u = 0;                              // zero out the memory for the f32
    i32 result = sscanf(str, "%hu", u);  // look for the floats using the format string input to id where the values should be in the string
    return result != -1;
}

// @brief attempts to parse a 32 bit unsigned integer from the provided string
// @param str the strin to parse from.
// @param u a pointer to the int to write to
// true if parsed successfully, false otherwise
b8 string_to_u32(char* str, u32* u) {
    if (!str) {        // check that a string had been input
        return false;  // fail
    }

    *u = 0;                             // zero out the memory for the f32
    i32 result = sscanf(str, "%u", u);  // look for the floats using the format string input to id where the values should be in the string
    return result != -1;
}

// @brief attempts to parse a 64 bit unsigned integer from the provided string
// @param str the strin to parse from.
// @param u a pointer to the int to write to
// true if parsed successfully, false otherwise
b8 string_to_u64(char* str, u64* u) {
    if (!str) {        // check that a string had been input
        return false;  // fail
    }

    *u = 0;                               // zero out the memory for the f32
    i32 result = sscanf(str, "%llu", u);  // look for the floats using the format string input to id where the values should be in the string
    return result != -1;
}

// @brief attempts to parse a boolean from the provided string
// "true" or "1" are considered true, everthing else is false
// @param str the string to parse from.
// @param b a pointer to the int to write to
// true if parsed successfully, false otherwise
b8 string_to_bool(char* str, b8* b) {
    if (!str) {        // check that a string had been input
        return false;  // fail
    }

    *b = strings_equal(str, "1") || strings_equali(str, "true");
    return *b;
}

// @brief splits the given string by the delimiter provided and stores in the provided darray.  optionally trims each array.
// NOTE: a string allocation occurs for each entry and must be freed by the caller
// @param str the string to be split
// @param delimiter the character to split by
// @param str_darray a pointer to a darray of char arrays to hold the entries NOTE: must be a darray
// @param trim_entries trims each entry if true
// @param include_empty indicates if empty entries should be included
// @return the number of entries yielded by the split operation
u32 string_split(const char* str, char delimeter, char*** str_darray, b8 trim_entries, b8 include_empty) {
    if (!str || !str_darray) {
        return 0;
    }

    char* result = 0;
    u32 trimmed_length = 0;
    u32 entry_count = 0;
    u32 length = string_length(str);
    char buffer[16384];  // if a single entry goes beyond this, well... just dont do that
    u32 current_length = 0;
    // iterate each character until a delimiter is reached
    for (u32 i = 0; i < length; ++i) {
        char c = str[i];

        // found delimeter, finalize the string
        if (c == delimeter) {
            buffer[current_length] = 0;
            result = buffer;
            trimmed_length = current_length;
            // trim if applicable
            if (trim_entries && current_length > 0) {
                result = string_trim(result);
                trimmed_length = string_length(result);
            }
            // add new entry
            if (trimmed_length > 0 || include_empty) {
                char* entry = kallocate(sizeof(char) * (trimmed_length + 1), MEMORY_TAG_STRING);
                if (trimmed_length == 0) {
                    entry[0] = 0;
                } else {
                    string_ncopy(entry, result, trimmed_length);
                    entry[trimmed_length] = 0;
                }
                char** a = *str_darray;
                darray_push(a, entry);
                *str_darray = a;
                entry_count++;
            }

            // clear the buffer
            kzero_memory(buffer, sizeof(char) * 16384);
            current_length = 0;
            continue;
        }

        buffer[current_length] = c;
        current_length++;
    }

    // at the end of the string. if any chars are qued up, read them.
    result = buffer;
    trimmed_length = current_length;
    // trim if applicable
    if (trim_entries && current_length > 0) {
        result = string_trim(result);
        trimmed_length = string_length(result);
    }
    // add a new entry
    if (trimmed_length > 0 || include_empty) {
        char* entry = kallocate(sizeof(char) * (trimmed_length + 1), MEMORY_TAG_STRING);
        if (trimmed_length == 0) {
            entry[0] = 0;
        } else {
            string_ncopy(entry, result, trimmed_length);
            entry[trimmed_length] = 0;
        }
        char** a = *str_darray;
        darray_push(a, entry);
        *str_darray = a;
        entry_count++;
    }

    return entry_count;
}

// @brief cleans up string allocations in str_darray, but does not free the darray itself
// @param str_darray the darray to be cleaned up
void string_cleanup_split_array(char** str_darray) {
    if (str_darray) {
        u32 count = darray_length(str_darray);
        // free each string
        for (u32 i = 0; i < count; ++i) {
            u32 len = string_length(str_darray[i]);
            kfree(str_darray[i], sizeof(char) * (len + 1), MEMORY_TAG_STRING);
        }

        // clear the darray
        darray_clear(str_darray);
    }
}

// @brief appends append to source and returns a new string
// @param dest the destination string
// @param source the string to be appended to
// @param append the string to append to source
// @returns a new string containing the concatenation of the two strings
void string_append_string(char* dest, const char* src, const char* append) {
    sprintf(dest, "%s%s", src, append);
}

// @brief appends the supplied integer to source and outputs to dest
// @brief dest the destination for the string
// @param source the string to be appended to
// @param i the interger to be appended
void string_append_int(char* dest, const char* source, i64 i) {
    sprintf(dest, "%s%lli", source, i);
}

// @brief appends the supplied float to source and outputs to dest
// @brief dest the destination for the string
// @param source the string to be appended to
// @param f the float to be appended
void string_append_float(char* dest, const char* source, f32 f) {
    sprintf(dest, "%s%f", source, f);
}

// @brief appends the supplied boolean (as either "true" or "false") to source and outputs to dest
// @brief dest the destination for the string
// @param source the string to be appended to
// @param b the boolean to be appended
void string_append_bool(char* dest, const char* source, b8 b) {
    sprintf(dest, "%s%s", source, b ? "true" : "false");
}

// @brief appends the supplied character to source and outputs to dest
// @brief dest the destination for the string
// @param source the string to be appended to
// @param c the character to be appended
void string_append_char(char* dest, const char* source, char c) {
    sprintf(dest, "%s%c", source, c);
}

// @brief extracts the directory from a full file path
// @param dest the destination for the path
// @param path the full path to extract from
void string_directory_from_path(char* dest, const char* path) {
    u64 length = strlen(path);
    for (i32 i = length; i >= 0; --i) {
        char c = path[i];
        if (c == '/' || c == '\\') {
            strncpy(dest, path, i + 1);
            return;
        }
    }
}

// @brief extracts the filename (including file extension) from a full file path
// @param dest the destination for the filename
// @param path the full path to extract from
void string_filename_from_path(char* dest, const char* path) {
    u64 length = strlen(path);
    for (i32 i = length; i >= 0; --i) {
        char c = path[i];
        if (c == '/' || c == '\\') {
            strcpy(dest, path + i + 1);
            return;
        }
    }
}

// @brief extracts the filename (excluding file extension) from a full file path
// @param dest the destination for the filename
// @param path the full path to extract from
void string_filename_no_extension_from_path(char* dest, const char* path) {
    u64 length = strlen(path);
    u64 start = 0;
    u64 end = 0;
    for (i32 i = length; i >= 0; --i) {
        char c = path[i];
        if (end == 0 && c == '.') {
            end = i;
        }
        if (start == 0 && (c == '/' || c == '\\')) {
            start = i + 1;
            break;
        }
    }

    string_mid(dest, path, start, end - start);
}