#include "filesystem.h"

#include "core/logger.h"
#include "core/kmemory.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

b8 filesystem_exists(const char* path) {
#ifdef _MSC_VER
    struct _stat buffer;
    return _stat(path, &buffer);
#else
    struct stat buffer;
    return stat(path, &buffer) == 0;
#endif
}

b8 filesystem_open(const char* path, file_modes mode, b8 binary, file_handle* out_handle) {
    out_handle->is_valid = false;  // reset out handle valid check to false
    out_handle->handle = 0;        // reset the handle
    const char* mode_str;          // define a character pointer to mode_str - create a string for the mode

    if ((mode & FILE_MODE_READ) != 0 && (mode & FILE_MODE_WRITE) != 0) {         // if both read and write input
        mode_str = binary ? "w+b" : "w+";                                        // if binary is true then the mode sting is "w+b", otherwise its "w+"
    } else if ((mode & FILE_MODE_READ) != 0 && (mode & FILE_MODE_WRITE) == 0) {  // if only read is passed in
        mode_str = binary ? "rb" : "r";                                          // if binary is true then the mode string is "rb", otherwise its "r"
    } else if ((mode & FILE_MODE_READ) == 0 && (mode & FILE_MODE_WRITE) != 0) {  // if only write is passed in
        mode_str = binary ? "wb" : "w";                                          // if binary is true the the mode string is "wb", otherwise its "w"
    } else {                                                                     // if an unknown or no mode type is passed in
        KERROR("Invalid mode passed while trying to open file: '%s'", path);     // throw error
        return false;                                                            // boot out
    }

    // attempt to open the file
    FILE* file = fopen(path, mode_str);            // use c funtion fopen, pass in the filepath and the mode to use, store a pointer to the data produced
    if (!file) {                                   // if there is nothing at the pointer
        KERROR("Error opening file: '%s'", path);  // throw an error
        return false;                              // boot out with failure
    }

    // was successful
    out_handle->handle = file;    // store a pointer to the file handle
    out_handle->is_valid = true;  // set is valid to true

    return true;
}

void filesystem_close(file_handle* handle) {
    if (handle->handle) {               // if there is actully a handle passed in
        fclose((FILE*)handle->handle);  // call c function fclose to close the file
        handle->handle = 0;             // reset the file handle to 0
        handle->is_valid = false;       // reset is valid to false
    }
}

// i am  a little lost on this one - need to look up
b8 filesystem_read_line(file_handle* handle, u64 max_length, char** line_buf, u64* out_line_length) {
    if (handle->handle && line_buf && out_line_length && max_length > 0) {  // if there is handle on the handle, line buff and out line length have been entered, and the max length is greater than 0
        char* buf = *line_buf;                                              // get a character array, by dereferencing line buf
        if (fgets(buf, max_length, (FILE*)handle->handle) != 0) {
            *out_line_length = strlen(*line_buf);
            return true;
        }
    }
    // if there isnt a handle
    return false;  // wont work
}

b8 filesystem_write_line(file_handle* handle, const char* text) {
    if (handle->handle) {                                 // if there is handle on the handle
        i32 result = fputs(text, (FILE*)handle->handle);  // use the c function fputs to put the provided text at the end of the file passed in
        if (result != EOF) {                              // if it wasnt tagged as end of the file
            result = fputc('\n', (FILE*)handle->handle);  // use the c function fputc to append a new line(like hitting enter new line) to the end of the file
        }

        // make sure to flush the stream so it is written to the file immediately
        // this prevents data loss in the event of a crash
        fflush((FILE*)handle->handle);
        return result != EOF;  // indicates success, hopefully understand better when we strat utilising
    }
    // if there wasnt a handle to a file handle
    return false;
}

b8 filesystem_read(file_handle* handle, u64 data_size, void* out_data, u64* out_bytes_read) {
    if (handle->handle && out_data) {                                            // if there is handle on the handle, and a pointer to a place to hold the data
                                                                                 // dereference out bytes read, and store the size from calling c func fread
        *out_bytes_read = fread(out_data, 1, data_size, (FILE*)handle->handle);  // pass it the place to hold the data, only 1 file, the size of the file, and a file pointer to the handle
        if (*out_bytes_read != data_size) {                                      // if the size returned was not the same as what was returned
            return false;                                                        // if faile
        }
        // if the sizes match was a success
        return true;
    }
    // if there wasnt a handle to a file handle
    return false;
}

b8 filesystem_read_all_bytes(file_handle* handle, u8** out_bytes, u64* out_bytes_read) {
    if (handle->handle) {  // if there is handle on the handle
        // file size
        fseek((FILE*)handle->handle, 0, SEEK_END);  // use c func to seek to the end of the file
        u64 size = ftell((FILE*)handle->handle);    // at the end of the file, use the ftell c func to tell how long the file is
        rewind((FILE*)handle->handle);              // use the c func rewind to go back to the beginning of the file

        *out_bytes = kallocate(sizeof(u8) * size, MEMORY_TAG_STRING);  // allocate some memory the size of a u8 times the size of the file
        // dereference out bytes read, and store the size from calling c func fread
        *out_bytes_read = fread(*out_bytes, 1, size, (FILE*)handle->handle);  // pass it the place to hold the data, only 1 file, the size of the file, and a file pointer to the handle
        if (*out_bytes_read != size) {                                        // if the out bytes read is not the same as the filesize
            return false;                                                     // boot outt failed
        }
        // if success
        return true;
    }
    // if there wasnt a handle to a file handle
    return false;
}

b8 filesystem_write(file_handle* handle, u64 data_size, const void* data, u64* out_bytes_written) {
    if (handle->handle) {                                                        // if there is handle on the handle
        *out_bytes_written = fwrite(data, 1, data_size, (FILE*)handle->handle);  // use the c func fwrite to write to the file, pass it the data to write, one file for now, the size of the data, and a file pointer to the handle store the size in out bytes written
        if (*out_bytes_written != data_size) {                                   // if the data written doesnt match the size input
            return false;                                                        // boot out failed
        }
        // if the sizes were the same, was a success
        // make sure to flush the stream so it is written to the file immediately
        // this prevents data loss in the event of a crash
        fflush((FILE*)handle->handle);
        return true;
    }
    // if there wasnt a handle to a file handle
    return false;
}