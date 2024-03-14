// stands for dynamic array
#pragma once

#include "defines.h"

// memory layout
// u64 capacity = number elements that can be held
// u64 lenth = number of elements currently contained
// u64 stride = size of each element in bytes
// void* elements

enum {
    DARRAY_CAPACITY,
    DARRAY_LENGTH,
    DARRAY_STRIDE,
    DARRAY_FIELD_LENGTH
};

KAPI void* _darray_create(u64 length, u64 stride);  // pass in the length - number of elements in new array and how large each element is
KAPI void _darray_destroy(void* array);             // void pointer to the array

// need a way to access these to get info and set info, getter and setter
KAPI u64 _darray_field_get(void* array, u64 field);
KAPI void _darray_field_set(void* array, u64 field, u64 value);

// automattically size the array for what is being sent in?
KAPI void* _darray_resize(void* array);

KAPI void* _darray_push(void* array, const void* value_ptr);  // pass in the array and the value to push into the array - if needs resizing call resize func
KAPI void _darray_pop(void* array, void* dest);               // sets value of dest to whatever was the last entry. think this is stack stuff

// here are ways to push and pop stuff into and out of the middle of the stack - do the same as before but takes in an index and performs the function at that index
KAPI void* _darray_pop_at(void* array, u64 index, void* dest);
KAPI void* _darray_insert_at(void* array, u64 index, void* value_ptr);

#define DARRAY_DEFAULT_CAPACITY 1
#define DARRAY_RESIZE_FACTOR 2

// macros that are actually used to call all of these functions
#define darray_create(type) \
    _darray_create(DARRAY_DEFAULT_CAPACITY, sizeof(type))  // default capcity of one

#define darray_reserve(type, capacity) \
    _darray_create(capacity, sizeof(type))  // allows you to pass in a capacity

#define darray_destroy(array) _darray_destroy(array);

// want the ability to pass in whatever type we want a this handles this
#define darray_push(array, value)           \
    {                                       \
        typeof(value) temp = value;         \
        array = _darray_push(array, &temp); \
    }

// NOTE: could use __auto_type for temp above, but intellisense for vs code flags it as an unknown type. this works just fine though

#define darray_pop(array, value_ptr) \
    _darray_pop(array, value_ptr)

// same as the push one, but with the index passed through as well
#define darray_insert_at(array, index, value)           \
    {                                                   \
        typeof(value) temp = value;                     \
        array = _darray_insert_at(array, index, &temp); \
    }

#define darray_pop_at(array, index, value_ptr) \
    _darray_pop_at(array, index, value_ptr)

// set the internal length of the array to zero
#define darray_clear(array) \
    _darray_field_set(array, DARRAY_LENGTH, 0)

// need a way to check the capacity of an array
#define darray_capacity(array) \
    _darray_field_get(array, DARRAY_CAPACITY)

// and a way to get the number of elements in an array
#define darray_length(array) \
    _darray_field_get(array, DARRAY_LENGTH)

// and a way to get the size of the elements in bytes
#define darray_stride(array) \
    _darray_field_get(array, DARRAY_STRIDE)

// and a way to set the number of elements in an array
#define darray_length_set(array, value) \
    _darray_field_set(array, DARRAY_LENGTH, value)
