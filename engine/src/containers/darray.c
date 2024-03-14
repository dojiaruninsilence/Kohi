#include "containers/darray.h"

#include "core/kmemory.h"
#include "core/logger.h"

void* _darray_create(u64 length, u64 stride) {
    u64 header_size = DARRAY_FIELD_LENGTH * sizeof(u64);                      // calc header size. # of fields * how big they are
    u64 array_size = length * stride;                                         // total size of elements is # of elements * how big they are
    u64* new_array = kallocate(header_size + array_size, MEMORY_TAG_DARRAY);  // create new array and allocate memory using kallocate
    kset_memory(new_array, 0, header_size + array_size);                      // zero out the memory just to be sure
    new_array[DARRAY_CAPACITY] = length;
    new_array[DARRAY_LENGTH] = 0;
    new_array[DARRAY_STRIDE] = stride;
    return (void*)(new_array + DARRAY_FIELD_LENGTH);  // slide pointer 24 bytes foreward cast it to a void pointer and return it, witout the header info attached
}

void _darray_destroy(void* array) {
    u64* header = (u64*)array - DARRAY_FIELD_LENGTH;                                 // get the header location by getting pointer to array and subtracting the total length of the array itself -- get the original position
    u64 header_size = DARRAY_FIELD_LENGTH * sizeof(u64);                             // calc the header size
    u64 total_size = header_size + header[DARRAY_CAPACITY] * header[DARRAY_STRIDE];  // get the total size to be freed
    kfree(header, total_size, MEMORY_TAG_DARRAY);                                    // free the header and array? confused a bit here
}

u64 _darray_field_get(void* array, u64 field) {       // takes in a pointer to an array and the index of the field
    u64* header = (u64*)array - DARRAY_FIELD_LENGTH;  // above i got this wrong this says to move back 3 8 byte sections i believe he said, keep both for now
    return header[field];                             // return the index of whatever field was passed into it
}

void _darray_field_set(void* array, u64 field, u64 value) {  // same as above but passes in a value as well and sets the index to that
    u64* header = (u64*)array - DARRAY_FIELD_LENGTH;         // same as above
    header[field] = value;                                   // set the index of the field that was passed in to value
}

void* _darray_resize(void* array) {
    u64 length = darray_length(array);  // current length
    u64 stride = darray_stride(array);
    void* temp = _darray_create(                          // create a new array
        (DARRAY_RESIZE_FACTOR * darray_capacity(array)),  // current capacity times the resize factor
        stride);                                          // passes in the stride
    kcopy_memory(temp, array, length * stride);           // copy form the old array to temp, which is now the new array

    _darray_field_set(temp, DARRAY_LENGTH, length);  // set the length of the new array
    _darray_destroy(array);                          // destroy original array
    return temp;                                     // new array with new length
}
void* _darray_push(void* array, const void* value_ptr) {
    u64 length = darray_length(array);
    u64 stride = darray_stride(array);
    if (length >= darray_capacity(array)) {  // if the capacity isnt already bigger than the length
        array = _darray_resize(array);       // then there needs to be a resize
    }

    u64 addr = (u64)array;                                // get the address of the array
    addr += (length * stride);                            // move the address foreward by the entire length of the array
    kcopy_memory((void*)addr, value_ptr, stride);         // copy the value in value ptr of stride size
    _darray_field_set(array, DARRAY_LENGTH, length + 1);  // increment the length of the array by one
    return array;
}

void _darray_pop(void* array, void* dest) {
    u64 length = darray_length(array);
    u64 stride = darray_stride(array);
    u64 addr = (u64)array;
    addr += ((length - 1) * stride);                      // do same as above but minus one
    kcopy_memory(dest, (void*)addr, stride);              // copy into the new pos, removing the last index
    _darray_field_set(array, DARRAY_LENGTH, length - 1);  // decrement the length field of the array by one
}
void* _darray_pop_at(void* array, u64 index, void* dest) {
    u64 length = darray_length(array);
    u64 stride = darray_stride(array);
    if (index >= length) {
        KERROR("Index is outside the bounds of this array! Length: %i, index: %index", length, index);
        return array;
    }

    u64 addr = (u64)array;
    kcopy_memory(dest, (void*)(addr + (index * stride)), stride);  // copy to the destination from the address plus the index times the stride cast that to a void pointer and give it the size of stride

    // if not on the lase element, snip out the entry and copy the rest inward.
    if (index != length - 1) {
        kcopy_memory(  // copies the entire block, with the inward movement incorporated?
            (void*)(addr + (index * stride)),
            (void*)(addr + ((index + 1) * stride)),
            stride * (length - index));
    }

    _darray_field_set(array, DARRAY_LENGTH, length - 1);  // decrement the length
    return array;
}

void* _darray_insert_at(void* array, u64 index, void* value_ptr) {
    u64 length = darray_length(array);
    u64 stride = darray_stride(array);
    if (index >= length) {
        KERROR("Index is outside the bounds of this array! Length: %i, index: %index", length, index);
        return array;
    }
    if (length >= darray_capacity(array)) {
        array = _darray_resize(array);
    }

    u64 addr = (u64)array;

    // if not on the last element, copy the rest outward.
    if (index != length - 1) {
        kcopy_memory(  // copies the entire block, with the outward movement incorporated?
            (void*)(addr + ((index + 1) * stride)),
            (void*)(addr + (index * stride)),
            stride * (length - index));
    }

    // set the value at the index
    kcopy_memory((void*)(addr + (index * stride)), value_ptr, stride);

    _darray_field_set(array, DARRAY_LENGTH, length + 1);  // increment the length
    return array;
}