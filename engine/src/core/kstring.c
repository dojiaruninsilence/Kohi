#include "core/kstring.h"
#include "core/kmemory.h"

#include <string.h>

u64 string_length(const char* str) {
    return strlen(str);
}

char* string_duplicate(const char* str) {
    u64 length = string_length(str);                        // get length of string to determine how much memory to reserve
    char* copy = kallocate(length + 1, MEMORY_TAG_STRING);  // allocate a new character array with lenght +1
    kcopy_memory(copy, str, length + 1);
    return copy;
}
