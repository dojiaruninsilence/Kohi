#include "core/kstring.h"
#include "core/kmemory.h"

#include <string.h>
#include <stdio.h>
#include <stdarg.h>

u64 string_length(const char* str) {
    return strlen(str);
}

char* string_duplicate(const char* str) {
    u64 length = string_length(str);                        // get length of string to determine how much memory to reserve
    char* copy = kallocate(length + 1, MEMORY_TAG_STRING);  // allocate a new character array with lenght +1
    kcopy_memory(copy, str, length + 1);
    return copy;
}

// case sensitive string comparison. true if they are the same otherwise its false
b8 strings_equal(const char* str0, const char* str1) {
    return strcmp(str0, str1) == 0;  // gonna utilize the c library command to compare strings for now
}

// need to look upo these variadic arguments
// performs string formatting to dest given format string and parameters
// pass in a pointer to a destination that is large enough to hold the final string, the format to use, aand the dots represent any number of arguments to throw in
KAPI i32 string_format(char* dest, const char* format, ...) {
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
KAPI i32 string_format_v(char* dest, const char* format, void* va_listp) {
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