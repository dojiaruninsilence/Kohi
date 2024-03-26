#include "vulkan_shader_utils.h"

#include "core/kstring.h"
#include "core/logger.h"
#include "core/kmemory.h"

#include "platform/filesystem.h"

// create a vulkan shader module
b8 create_shader_module(
    vulkan_context* context,                  // pass in a pointer to the context
    const char* name,                         // pass in the shader name object, this will concantenate with the
    const char* type_str,                     // type string(vert, frag, ect), and the extension to get the filename
    VkShaderStageFlagBits shader_stage_flag,  // also pass in the vulkan stage type(vertex, fragment, ect)
    u32 stage_index,                          // the stage index(i)
    vulkan_shader_stage* shader_stages) {     // and the array of vulkan shader stages
    // build the filename
    char file_name[512];                                                   // create an array of characters 512 long called file name -- buffer for the string format
    string_format(file_name, "assets/shaders/%s.%s.spv", name, type_str);  // use our string format, pass in the name, and type string, and a start string, concat to "assets/shaders/[name value].[type_str value].spv" and stor in filename

    kzero_memory(&shader_stages[stage_index].create_info, sizeof(VkShaderModuleCreateInfo));     // zero out the memory in the shader stages array, at stage index(i), use the default size of a vulkan shader module create info struct
    shader_stages[stage_index].create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;  // pass the vulkan create info struct the vulkan macro to input the default values

    // obtain the file handle
    file_handle handle;  // define the handle
    // run filesystem open function  pass in the file name from above, set to read mode, the file will be in binary, and an address to the handle
    if (!filesystem_open(file_name, FILE_MODE_READ, true, &handle)) {  // if it returns a false value
        KERROR("Unable to read shader module: %s.", file_name);        // throw an error
        return false;                                                  // boot out
    }

    // read the entire file as binary
    u64 size = 0;         // define a u64 size and set to 0
    u8* file_buffer = 0;  // define a u8 pointer file buffer and set to 0
    // run filesystem read all bytes, pass it the address to the handle, the address to the filebuffer, and the address to the size
    if (!filesystem_read_all_bytes(&handle, &file_buffer, &size)) {     // if it fails
        KERROR("Unable to binary read shader module: %s.", file_name);  // throw an error
        return false;                                                   // boot out
    }
    shader_stages[stage_index].create_info.codeSize = size;            // set the code size in create info for the shader stage at index i to the size produced by the read all bytes
    shader_stages[stage_index].create_info.pCode = (u32*)file_buffer;  // set the pcode in create info for the shader stage at index i to a pointer to the file buffer

    // close the file
    filesystem_close(&handle);

    // call the vulkan function to create a shader module and run it against vk check
    VK_CHECK(vkCreateShaderModule(
        context->device.logical_device,           // pass it the logical device
        &shader_stages[stage_index].create_info,  // pass it the create info filled out before
        context->allocator,                       // the memory allocation stuffs
        &shader_stages[stage_index].handle));     // and a handle to the shader module being created

    // shader stage info
    kzero_memory(&shader_stages[stage_index].shader_stage_create_info, sizeof(VkPipelineShaderStageCreateInfo));      // zero out all the create info stuff for the shader stage
    shader_stages[stage_index].shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;  // set the stucture defaults using the vulkan macro
    shader_stages[stage_index].shader_stage_create_info.stage = shader_stage_flag;                                    // pass in the stage type(shader, fragment, ect)
    shader_stages[stage_index].shader_stage_create_info.module = shader_stages[stage_index].handle;                   // pass in the handle to the resulting shader module
    shader_stages[stage_index].shader_stage_create_info.pName = "main";                                               // give the shader module the name of main, for main shadr i would guess - this is the entry point to the shader itself

    // if (file_buffer) {                                             // if a file buffer exists
    //     kfree(file_buffer, sizeof(u8) * size, MEMORY_TAG_STRING);  // free the memory it was pointing to, using the size of a u8 times the size of the file
    //     file_buffer = 0;                                           // reset the file buffer pointer
    // }

    // if all was a success
    return true;
}