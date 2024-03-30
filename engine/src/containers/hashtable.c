#include "hashtable.h"

#include "core/kmemory.h"
#include "core/logger.h"

u64 hash_name(const char* name, u32 element_count) {
    // a multiplier to use when generating a hash. prime to hopefully avoid collisions
    static const u64 multiplier = 97;

    unsigned const char* us;  // an array of characters
    u64 hash = 0;             // store the running total of the hash

    for (us = (unsigned const char*)name; *us; us++) {  // iterate through all the characters
        hash = hash * multiplier + *us;                 // each iteration multiply the running total by the multiplier then add the next char
    }

    // mod it against the size of the table
    hash %= element_count;

    return hash;
}

void hashtable_create(u64 element_size, u32 element_count, void* memory, b8 is_pointer_type, hashtable* out_hashtable) {
    // make sure that all the required fields have been given actual values
    if (!memory || !out_hashtable) {
        KERROR("hashtable_create failed! pointer to memory and out_hashtable are required.");
        return;
    }
    if (!element_count || !element_size) {
        KERROR("element_size and element_count must be a positive non zero value.");
        return;
    }

    // TODO: might want to require an allocator and allocate this memory instead
    out_hashtable->memory = memory;
    out_hashtable->element_count = element_count;
    out_hashtable->element_size = element_size;
    out_hashtable->is_pointer_type = is_pointer_type;
    kzero_memory(out_hashtable->memory, element_size * element_count);
}

void hashtable_destroy(hashtable* table) {
    if (table) {
        // TODO: if using allocator above, free memory here
        kzero_memory(table, sizeof(hashtable));
    }
}

b8 hashtable_set(hashtable* table, const char* name, void* value) {
    // make sure that all of the required fields have been filled in and that the table is not a pointer table
    if (!table || !name || !value) {
        KERROR("hashtable_set requires a table, name and value to exist.");
        return false;
    }
    if (table->is_pointer_type) {
        KERROR("hashtable_set should not be used with tables that have pointer types. use hashtable_set_ptr instead.");
        return false;
    }

    // hash the name using our function
    u64 hash = hash_name(name, table->element_count);
    kcopy_memory(table->memory + (table->element_size * hash), value, table->element_size);  // push the hashed name and value into the table
    return true;
}

b8 hashtable_set_ptr(hashtable* table, const char* name, void** value) {
    // make sure that all of the required fields have been filled in and that the table is a pointer table
    if (!table || !name) {
        KERROR("hashtable_set_ptr requires a table and name and to exist.");
        return false;
    }
    if (!table->is_pointer_type) {
        KERROR("hashtable_set_ptr should not be used with tables that don not have pointer types. use hashtable_set instead.");
        return false;
    }

    // hash the name using our function
    u64 hash = hash_name(name, table->element_count);
    ((void**)table->memory)[hash] = value ? *value : 0;  // push the hashed name and value into the table, if value was 0 unset the entrie at that point
    return true;
}

b8 hashtable_get(hashtable* table, const char* name, void* out_value) {
    // make sure that all of the required fields have been filled in and that the table is not a pointer table
    if (!table || !name || !out_value) {
        KERROR("hashtable_get requires a table, name and value to exist.");
        return false;
    }
    if (table->is_pointer_type) {
        KERROR("hashtable_get should not be used with tables that have pointer types. use hashtable_get_ptr instead.");
        return false;
    }

    // hash the name using our function
    u64 hash = hash_name(name, table->element_count);
    kcopy_memory(out_value, table->memory + (table->element_size * hash), table->element_size);  // copy the value in the table where the name is into the out value
    return true;
}

b8 hashtable_get_ptr(hashtable* table, const char* name, void** out_value) {
    // make sure that all of the required fields have been filled in and that the table is not a pointer table
    if (!table || !name || !out_value) {
        KERROR("hashtable_get requires a table, name and value to exist.");
        return false;
    }
    if (!table->is_pointer_type) {
        KERROR("hashtable_set_ptr should not be used with tables that don not have pointer types. use hashtable_set instead.");
        return false;
    }

    // hash the name using our function
    u64 hash = hash_name(name, table->element_count);
    *out_value = ((void**)table->memory)[hash];
    return *out_value != 0;
}

b8 hashtable_fill(hashtable* table, void* value) {
    if (!table || !value) {
        KWARN("hashtable_fill requires table and value to exist.");
        return false;
    }
    if (table->is_pointer_type) {
        KERROR("hashtable_fill should not be used with tables that have pointer types.");
        return false;
    }

    for (u32 i = 0; i < table->element_count; ++i) {                                          // iterate through all the elements in the table
        kcopy_memory(table->memory + (table->element_size * i), value, table->element_size);  // copy the provided value into all of the elements of the table
    }

    return true;
}