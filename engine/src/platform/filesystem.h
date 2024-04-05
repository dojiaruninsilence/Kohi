#pragma once

#include "defines.h"

// holds a handle to a file
typedef struct file_handle {
    // opaque handle to internal file handle
    void* handle;  // a pointer to the file handle - will be different per platform
    b8 is_valid;   // is the file handle valid
} file_handle;

// an enum of file modes - can be combined when needed
typedef enum file_modes {
    FILE_MODE_READ = 0x1,
    FILE_MODE_WRITE = 0x2
} file_modes;

// checks if a file with the given path exists
// @param path the path of the file to be checked
// @returns true if exists, otherwise false
KAPI b8 filesystem_exists(const char* path);

// attempt to open file located at path
// @param path the path of the file to be opened
// @param mode mode flags for the file when opened (read/write). see file modes enum in filesystem.h
// @param binary indicates if the file should be opened in binary mode
// @param out_handle a pointer to a file_handle structure which holds the handle info
// @returns true if opened successfully otherwise false
KAPI b8 filesystem_open(const char* path, file_modes mode, b8 binary, file_handle* out_handle);

// closes the provided handle to a file
// @param handle a pointer to a file handle structure which holds the handle to be closed
KAPI void filesystem_close(file_handle* handle);

// reads up to a newline or EOF.
// @param handle a pointer to a file_handle structure
// @param max_length the maximum length to be read from the line
// @param line_buf a pointer to a character array populated by this method. must already be allocated
// @param out_line_length a pointer to hold the line length read from the file
// @returns true is successful, otherwise false
KAPI b8 filesystem_read_line(file_handle* handle, u64 max_length, char** line_buff, u64* out_line_length);

// writes text to the provided file, appending a '\n' afterward
// @param handle a pointer to a file_handle structure
// @param text the text to be written
// @returns true if successful, otherwise false
KAPI b8 filesystem_write_line(file_handle* handle, const char* text);

// reads up to data size bytes of data into out bytes read
// allocates *out_data, which must be freed by the caller
// @param handle a pointer to a file_handle struct
// @param data_size the number of bytes to read
// @param out_data a pointer to a block of memory to be populated by this method
// @param out_bytes_read a pointer to a number which will be populated with the number of bytes actually read from the file
// @ returns true if successful, otherwise false
KAPI b8 filesystem_read(file_handle* handle, u64 data_size, void* out_data, u64* out_bytes_read);

// reads up to data_size bytes of data into out_bytes_read.
// allocates *out_bytes, which must be freed by the caller
// @param handle a pointer to a file_handle structure
// @param out_bytes a pointer to a byte array which will be allocated and populated by this method
// @param out_bytes_read a pointer to a number which will be populated with the number of bytes actually read from the file
// @returns true if successful, otherwise false
KAPI b8 filesystem_read_all_bytes(file_handle* handle, u8** out_bytes, u64* out_bytes_read);

// writes provided data to the file
// @param handle a pointer to a file_handle struct
// @param data_size the size of the data in bytes
// @param data the data to be written
// @param out_bytes_written a pointer to a number which will be populated with the number of bytes actually written to the file
// @returns true is a success, and false otherwise
KAPI b8 filesystem_write(file_handle* handle, u64 data_size, const void* data, u64* out_bytes_written);
