#pragma once

#include "defines.h"

// @brief represents a simple hashtable. members of this structure should not be modified
// outside of the functions associated with it
// for non pointer types, table retains a copy of the value. for pointer types, make sure to use the _ptr
// setter and getter. table does not take ownership of pointers or associated memory allocations,
// and should be managed externally
typedef struct hashtable {
    u64 element_size;    // the size of each element, everything in the hashtable will be the same size, makes it faster
    u32 element_count;   // maximum element count
    b8 is_pointer_type;  // is it a pointer type
    void* memory;        // memor bock to hold all of the elements in the hashtable
} hashtable;

// @brief creates a hashtable and stores it in out_hashtable
// @param element_size the size of each element in bytes
// @param element_count the maximum number of elements. cannot be resized
// @param memory a block of memory to be used. should be equal in size to element_size * element_count
// @param is_pointer_type indicates if this hashtable will hold pointer types
// @param out_hashtable a pointer to a hashtable in which to hold relevent data
KAPI void hashtable_create(u64 element_size, u32 element_count, void* memory, b8 is_pointer_type, hashtable* out_hashtable);

// @brief destroys the provided hashtable. does not release memory for pointer types
// @param table a pointer to the table to be destroyed
KAPI void hashtable_destroy(hashtable* table);

// @brief stores a copy of the data in value in the provided hashtable.
// only use for tables which were NOT created with is_pointer_type = true
// @param table a pointer to the table to get from. required
// @param name the name of the entry to set. required
// @param value the value to be set. required
// @return true, or false if a null pointer is passed
KAPI b8 hashtable_set(hashtable* table, const char* name, void* value);

// @brief stores a pointer as provided in value in the hashtable.
// only use for tables which were created with is_pointer_type = true
// @param table a pointer to the table to get from. required
// @param name the name of the entry to set. required
// @param value a pointer value to be set. can pass 0 to 'unset' an entry
// @return true, or false if a null pointer is passed or if the entry is 0
KAPI b8 hashtable_set_ptr(hashtable* table, const char* name, void** value);

// @brief obtains a copy of data present in the hashtable
// only use for tables which were NOT created with is_pointer_type = true
// @param table a pointer to the table to be retrieves from. required
// @param name the name of the entry to be retrieved. required
// @param value a pointer to store the retrieved value. required
// @return true, of false if a null pointer is passed
KAPI b8 hashtable_get(hashtable* table, const char* name, void* out_value);

// @brief obtains a copy of data present in the hashtable
// only use for tables which were created with is_pointer_type = true
// @param table a pointer to the table to be retrieves from. required
// @param name the name of the entry to be retrieved. required
// @param value a pointer to store the retrieved value. required
// @return true, of false if a null pointer is passed or the retrieved value is 0
KAPI b8 hashtable_get_ptr(hashtable* table, const char* name, void** out_value);

// @brief fills all the enries in the hashtable with the given value.
// useful when non existent names should return some default vlaue
// should not be used with pointer table types
// @param table a pointer to the table to be filled. required
// @param value the value to be filled with. required
// @return true if successful, otherwise false
KAPI b8 hashtable_fill(hashtable* table, void* value);