#include "vulkan_object_shader.h"

#include "core/logger.h"
#include "core/kmemory.h"
#include "math/math_types.h"
#include "math/kmath.h"

#include "renderer/vulkan/vulkan_shader_utils.h"
#include "renderer/vulkan/vulkan_pipeline.h"
#include "renderer/vulkan/vulkan_buffer.h"

#define BUILTIN_SHADER_NAME_OBJECT "Builtin.ObjectShader"  // this is part of the filename that we are going to load will be concatenated with the stage name and extension -- for custom shaders and such this will be different

// create a vulkan object shader, pass in a pointer to the context and a pointer to where the stucture for the shader will be held
b8 vulkan_object_shader_create(vulkan_context* context, texture* default_diffuse, vulkan_object_shader* out_shader) {
    // take and store a copy of the default texture pointers
    out_shader->default_diffuse = default_diffuse;

    // shader module initialize per stage
    char stage_type_strs[OBJECT_SHADER_STAGE_COUNT][5] = {"vert", "frag"};  // set up a short array of strings to obtain the various stage names that we will need for this shader
    // store the stage types - use the shader stage count for the size, and input vulkan macros for the types(vertex and fragment for now)
    VkShaderStageFlagBits stage_types[OBJECT_SHADER_STAGE_COUNT] = {VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT};

    for (u32 i = 0; i < OBJECT_SHADER_STAGE_COUNT; ++i) {  // iterate through all the stages
        // create shader modules - contains the logic for programmable shaders
        // use our funtion, pass in the context, the shader name object, the stage name at index i, the stage type at index i, and a pointer to the resulting shader stages array
        if (!create_shader_module(context, BUILTIN_SHADER_NAME_OBJECT, stage_type_strs[i], stage_types[i], i, out_shader->stages)) {  // if it fails
            KERROR("Unable to create %s shader module for '%s'.", stage_type_strs[i], BUILTIN_SHADER_NAME_OBJECT);                    // throw an error
            return false;                                                                                                             // boot out
        }
    }

    // global descriptors
    // descriptor set layout
    VkDescriptorSetLayoutBinding global_ubo_layout_binding;                        // define a descriptors sets layout binding
    global_ubo_layout_binding.binding = 0;                                         // binding at index 0
    global_ubo_layout_binding.descriptorCount = 1;                                 // only one for now
    global_ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;  // set to uniform buffer - tells it how the descriptor will be used
    global_ubo_layout_binding.pImmutableSamplers = 0;
    global_ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;  // for the vertex stage and only the vertex stage for now

    // create the vulkan struct for creating a descriptor set layout, use the macro to format and fill with default values
    VkDescriptorSetLayoutCreateInfo global_layout_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    global_layout_info.bindingCount = 1;                        // only one for now
    global_layout_info.pBindings = &global_ubo_layout_binding;  // struct created above
    // create the descriptor set layout - pass in the logical device, the info struct created above, memory allocation stuffs, and an address of where the descriptor set layout is going to be held
    VK_CHECK(vkCreateDescriptorSetLayout(context->device.logical_device, &global_layout_info, context->allocator, &out_shader->global_descriptor_set_layout));

    // setup the descriptor pool used for global items such as view/projection matrix
    VkDescriptorPoolSize global_pool_size;                              // define the global descriptor pool
    global_pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;          // set the type to uniform buffer - should match the type passed into the set layout
    global_pool_size.descriptorCount = context->swapchain.image_count;  // need a desctiptor for each swapchain image, so if we are triple buffering, we will need 3

    // create the vulkan struct for creating a descriptor pool, use the macro to format and fill with default values
    VkDescriptorPoolCreateInfo global_pool_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    global_pool_info.poolSizeCount = 1;                         // not the number of pools being created, but the number of pool size objects being passed in
    global_pool_info.pPoolSizes = &global_pool_size;            // pass in the pool sizes here
    global_pool_info.maxSets = context->swapchain.image_count;  // how many sets are going to be stored, so again for triple buffering this will be three
    // create the global descriptor pool
    VK_CHECK(vkCreateDescriptorPool(context->device.logical_device, &global_pool_info, context->allocator, &out_shader->global_descriptor_pool));

    // create local/object descriptors, and a local/object descriptor pool
    const u32 local_sampler_count = 1;
    VkDescriptorType descriptor_types[VULKAN_OBJECT_SHADER_DESCRIPTOR_COUNT] = {
        // create an array of descriptor types
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,          // binding 0 - uniform buffer
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,  // binding 1 - diffuse sampler layout
    };
    VkDescriptorSetLayoutBinding bindings[VULKAN_OBJECT_SHADER_DESCRIPTOR_COUNT];                           // create an array of descriptor set layouts, only one for now
    kzero_memory(&bindings, sizeof(VkDescriptorSetLayoutBinding) * VULKAN_OBJECT_SHADER_DESCRIPTOR_COUNT);  // zero out the memory for the array
    for (u32 i = 0; i < VULKAN_OBJECT_SHADER_DESCRIPTOR_COUNT; i++) {                                       // iterate through all the descriptors
        bindings[i].binding = i;                                                                            // pass in the binding at index i
        bindings[i].descriptorCount = 1;                                                                    // only one for now
        bindings[i].descriptorType = descriptor_types[i];                                                   // pass in the type at index i
        bindings[i].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;                                              // for the fragment shader
    }

    // create the vulkan struct for creating a descriptor set layout, use the macro to format and fill with default values
    VkDescriptorSetLayoutCreateInfo layout_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    layout_info.bindingCount = VULKAN_OBJECT_SHADER_DESCRIPTOR_COUNT;  // only one for now
    layout_info.pBindings = bindings;
    // create the descriptor set layout - pass in the logical device, the info struct created above, memory allocation stuffs, and an address of where the descriptor set layout is going to be held
    VK_CHECK(vkCreateDescriptorSetLayout(context->device.logical_device, &layout_info, 0, &out_shader->object_descriptor_set_layout));

    // local/object descriptor pool: used for object-specific items like diffuse colour
    VkDescriptorPoolSize object_pool_sizes[2];
    // the first section will be used for uniform buffers
    object_pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    object_pool_sizes[0].descriptorCount = VULKAN_OBJECT_MAX_OBJECT_COUNT;
    // the second section will be used for image samplers
    object_pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    object_pool_sizes[1].descriptorCount = local_sampler_count * VULKAN_OBJECT_MAX_OBJECT_COUNT;

    // create the vulkan struct for creating a descriptor pool, use the macro to format and fill with default values
    VkDescriptorPoolCreateInfo object_pool_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    object_pool_info.poolSizeCount = 2;
    object_pool_info.pPoolSizes = object_pool_sizes;
    object_pool_info.maxSets = VULKAN_OBJECT_MAX_OBJECT_COUNT;

    // create object descriptor pool
    VK_CHECK(vkCreateDescriptorPool(context->device.logical_device, &object_pool_info, context->allocator, &out_shader->object_descriptor_pool));

    // pipeline creation
    // define the viewport to pass into the pipeline
    VkViewport viewport;                                  // define a vulkan viewport
    viewport.x = 0.0f;                                    // x is set to 0
    viewport.y = (f32)context->framebuffer_height;        // y to the height of the framebuffer, because we have to flip the y axis - to match opengl
    viewport.width = (f32)context->framebuffer_width;     // pass in the framebuffer width
    viewport.height = -(f32)context->framebuffer_height;  // and height is negative, to flip the y axis - to match opengl
    viewport.minDepth = 0.0f;                             // min depth is 0, think these are 2d settings
    viewport.maxDepth = 1.0f;                             // max depth is 1

    // define the scissor - define what is displayed and what is clipped
    VkRect2D scissor;
    scissor.offset.x = scissor.offset.y = 0;              // set both x and y  offsets to 0, believe this means it lines up with the viewport
    scissor.extent.width = context->framebuffer_width;    // pass in the width
    scissor.extent.height = context->framebuffer_height;  // and height from the framebuffer

    // define the attributes - making this expandable
    u32 offset = 0;                                                             // as we add attributes, this will help us offset new ones in bytes
#define ATTRIBUTE_COUNT 2                                                       // define the attribute count
    VkVertexInputAttributeDescription attribute_descriptions[ATTRIBUTE_COUNT];  // create an array of vulkan attributes
    // position, texture coords
    VkFormat formats[ATTRIBUTE_COUNT] = {
        // create an array of vulkan formats, these are for the stuff passed to the glsl files
        VK_FORMAT_R32G32B32_SFLOAT,  // format for our only attribute at the moment - believe this is for a vec3 with 32 bit floating point elements
        VK_FORMAT_R32G32_SFLOAT};    // X and Y texture coords
    u64 sizes[ATTRIBUTE_COUNT] = {
        // create an array that will have size values, that correspond to the attributes at same index in format array
        sizeof(vec3),                                   //  vec3(position)
        sizeof(vec2)};                                  // texture coords
    for (u32 i = 0; i < ATTRIBUTE_COUNT; ++i) {         // iterate through the attributes
        attribute_descriptions[i].binding = 0;          // set binding index to zero
        attribute_descriptions[i].location = i;         // location to i - this location is the same location in glsl files, this is where it knows to send it
        attribute_descriptions[i].format = formats[i];  // set to corresponding format
        attribute_descriptions[i].offset = offset;      // pass in the offset
        offset += sizes[i];                             // increment the offset
    }

    // descriptor set layouts
    const i32 descriptor_set_layout_count = 2;
    // define the set layouts array
    VkDescriptorSetLayout layouts[2] = {
        out_shader->global_descriptor_set_layout,
        out_shader->object_descriptor_set_layout};

    // stages
    // NOTE: should match the number of shader->stages
    VkPipelineShaderStageCreateInfo stage_create_infos[OBJECT_SHADER_STAGE_COUNT];           // define an array vulkan shader stage create infos with stage count macro as the size
    kzero_memory(stage_create_infos, sizeof(stage_create_infos));                            // zero out the memory for the array
    for (u32 i = 0; i < OBJECT_SHADER_STAGE_COUNT; ++i) {                                    // iterate through the stage count
        stage_create_infos[i].sType = out_shader->stages[i].shader_stage_create_info.sType;  // copy over the stype
        stage_create_infos[i] = out_shader->stages[i].shader_stage_create_info;              // and the shader create infos
    }

    // create the graphics pipeline
    if (!vulkan_graphics_pipeline_create(  // and check to see if it fails
            context,                       // pass it the context
            &context->main_renderpass,     // the main renderpass
            ATTRIBUTE_COUNT,               // attribute count
            attribute_descriptions,        // attribute array
            descriptor_set_layout_count,   // no descriptors
            layouts,                       // no descriptors
            OBJECT_SHADER_STAGE_COUNT,     // give it the stage count
            stage_create_infos,            // the needed create infos
            viewport,                      // the viewport defined for the pipeline
            scissor,                       // the scissor
            false,                         // is not wire frame
            &out_shader->pipeline)) {      // and the address to where the pipline will be held
        KERROR("Failed to load graphics pipeline for object shader.");
        return false;
    }

    // create the uniform buffer
    if (!vulkan_buffer_create(                                                                                                 // check if it fails
            context,                                                                                                           // pass it the context
            sizeof(global_uniform_object),                                                                                     // use size of the global uniform object for the size
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,                                             // this will be a transfer destination, and will be used as a uniform buffer
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,  // will use local device memory for performance, but will be visible and coherent to the host
            true,                                                                                                              // bind as soon as its made
            &out_shader->global_uniform_buffer)) {                                                                             // the address of where the buffer will be held
        KERROR("Vulkan Buffer creation failed for object shader.");
        return false;
    }

    // allocate the global descriptor sets
    // define the global descriptor sets layouts array, with a length of 3, for triple buffering
    VkDescriptorSetLayout global_layouts[3] = {
        out_shader->global_descriptor_set_layout,  // use the same layout for all three descriptor sets
        out_shader->global_descriptor_set_layout,
        out_shader->global_descriptor_set_layout};

    // create the vulkan struct for creating a descriptor set allocation, use the macro to format and fill with default values
    VkDescriptorSetAllocateInfo alloc_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    alloc_info.descriptorPool = out_shader->global_descriptor_pool;  // pass it the pool created above
    alloc_info.descriptorSetCount = 3;                               // for triple buffering
    alloc_info.pSetLayouts = global_layouts;                         // pass in the array of layaouts from above
    // allocate the descriptor sets, pass in the logical device, the info just created, and the descriptor sets to allocate
    VK_CHECK(vkAllocateDescriptorSets(context->device.logical_device, &alloc_info, out_shader->global_descriptor_sets));

    // create the object uniform buffer
    if (!vulkan_buffer_create(                                                                                                 // check if it fails
            context,                                                                                                           // pass it the context
            sizeof(object_uniform_object),                                                                                     // use size of the uniform object for the size
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,                                             // this will be a transfer destination, and will be used as a uniform buffer
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,  // will use local device memory for performance, but will be visible and coherent to the host
            true,                                                                                                              // bind as soon as its made
            &out_shader->object_uniform_buffer)) {                                                                             // the address of where the buffer will be held
        KERROR("Material Instance Buffer creation failed for object shader.");
        return false;
    }

    return true;
}

// create a vulkan object shader, pass in a pointer to the context and a pointer to where the stucture for the shader is held
void vulkan_object_shader_destroy(vulkan_context* context, struct vulkan_object_shader* shader) {
    // convenience definition
    VkDevice logical_device = context->device.logical_device;

    // destroy object descriptor pool
    vkDestroyDescriptorPool(logical_device, shader->object_descriptor_pool, context->allocator);

    // destroy object descriptor set layouts
    vkDestroyDescriptorSetLayout(logical_device, shader->object_descriptor_set_layout, context->allocator);

    // destroy global uniform buffer
    vulkan_buffer_destroy(context, &shader->global_uniform_buffer);
    // destroy object uniform buffer
    vulkan_buffer_destroy(context, &shader->object_uniform_buffer);

    // destroy the pipeline
    vulkan_pipeline_destroy(context, &shader->pipeline);

    // destroy the global descriptor pool
    vkDestroyDescriptorPool(logical_device, shader->global_descriptor_pool, context->allocator);

    // destroy global descriptor set layouts
    vkDestroyDescriptorSetLayout(logical_device, shader->global_descriptor_set_layout, context->allocator);

    // destroy shader modules
    for (u32 i = 0; i < OBJECT_SHADER_STAGE_COUNT; ++i) {                                                     // iterate through all of the stages
        vkDestroyShaderModule(context->device.logical_device, shader->stages[i].handle, context->allocator);  // use the vulkan function to destoy each module
        shader->stages[i].handle = 0;                                                                         // and reset each of the handles
    }
}

// use a vulkan object shader, pass in a pointer to the context, and a pointer to where the shader struct is held
void vulkan_object_shader_use(vulkan_context* context, struct vulkan_object_shader* shader) {
    u32 image_index = context->image_index;  // get the current image index from the context
    // bind the pipline, pass in the graphics command buffer at the current image index, bind to graphics bind point of the pipeline, and an address to the pipline to bind
    vulkan_pipeline_bind(&context->graphics_command_buffers[image_index], VK_PIPELINE_BIND_POINT_GRAPHICS, &shader->pipeline);
}

// update the object shaders global state, , pass in a pointer to the context, and a pointer to where the shader struct is held
void vulkan_object_shader_update_global_state(vulkan_context* context, struct vulkan_object_shader* shader, f32 delta_time) {
    u32 image_index = context->image_index;                                                  // convenience
    VkCommandBuffer command_buffer = context->graphics_command_buffers[image_index].handle;  // define a command buffer to use
    VkDescriptorSet global_descriptor = shader->global_descriptor_sets[image_index];         // define a descriptor to update

    // bind the global descriptor set to be updated, pass in the command buffer, bind to the graphics pipeline, pass in the pipeline layout, first set is at index 0, only doing one set, pass in the descriptor set to update, 0 in both offset fields
    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shader->pipeline.pipeline_layout, 0, 1, &global_descriptor, 0, 0);

    // configure the descriptors for the given index
    u32 range = sizeof(global_uniform_object);  // range is the size of the global uniform object
    u64 offset = 0;                             // no offset for now

    // copy the data to the buffer, pass it the context, the buffer to load to, the offset and range generated above, no flags, and the data to load
    vulkan_buffer_load_data(context, &shader->global_uniform_buffer, offset, range, 0, &shader->global_ubo);

    VkDescriptorBufferInfo bufferInfo;                         // define a descriptor buffer info struct
    bufferInfo.buffer = shader->global_uniform_buffer.handle;  // pass it the handle to the global uniform buffer
    bufferInfo.offset = offset;                                // the offset
    bufferInfo.range = range;                                  // and range frm above

    // update descriptor sets
    VkWriteDescriptorSet descriptor_write = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};  // define a write descriptor set - this is what will contain the info to be written to the descriptor set
    descriptor_write.dstSet = shader->global_descriptor_sets[image_index];             // destination is the descriptor set tied to the current image
    descriptor_write.dstBinding = 0;                                                   // index 0, first one
    descriptor_write.dstArrayElement = 0;                                              // only one
    descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;               // for a uniform buffer
    descriptor_write.descriptorCount = 1;                                              // only one
    descriptor_write.pBufferInfo = &bufferInfo;                                        // an array even though only one for now

    // update the descriptor sets, pass in the logical device, only one set, pass in the info created above, dont make any copies
    vkUpdateDescriptorSets(context->device.logical_device, 1, &descriptor_write, 0, 0);
}

// this is only temporary, this will be moving
void vulkan_object_shader_update_object(vulkan_context* context, struct vulkan_object_shader* shader, geometry_render_data data) {
    u32 image_index = context->image_index;                                                  // convenience
    VkCommandBuffer command_buffer = context->graphics_command_buffers[image_index].handle;  // define a command buffer to use

    // push constants work kind of like a uniform except that they work without descriptor sets - used for frequently changing data - can be executed at any point, not only in a render pass
    // vulkan recomends using no more than 128 bytes for a push constant
    // push constants fuction, pass in the current graphics command buffer, the pipline layout, vertex stage, 0 offset, the size of a 4 x 4 matrix, and the address of the model
    vkCmdPushConstants(command_buffer, shader->pipeline.pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(mat4), &data.model);

    // obtain material data
    vulkan_object_shader_object_state* object_state = &shader->object_states[data.object_id];  // store a pointer to the objects id
    VkDescriptorSet object_descriptor_set = object_state->descriptor_sets[image_index];        // store the desciptor set for the object at image index

    // TODO: if needs update
    VkWriteDescriptorSet descriptor_writes[VULKAN_OBJECT_SHADER_DESCRIPTOR_COUNT];                          // define an array of descriptor sets for writing
    kzero_memory(descriptor_writes, sizeof(VkWriteDescriptorSet) * VULKAN_OBJECT_SHADER_DESCRIPTOR_COUNT);  // clear the memory of the descriptor sets
    u32 descriptor_count = 0;                                                                               // define a descriptor count with a value of zero
    u32 descriptor_index = 0;                                                                               // define a descriptor index with a value of zero

    // descriptor 0 - uniform buffer
    u32 range = sizeof(object_uniform_object);
    u32 offset = sizeof(object_uniform_object) * data.object_id;  // also the index into the array
    object_uniform_object obo;

    // experimental
    // TODO: get diffuse color from a material
    static f32 accumulator = 0.0f;              // define an accumulator
    accumulator += context->frame_delta_time;   // add the delta time to it every frame
    f32 s = (ksin(accumulator) + 1.0f) / 2.0f;  // scale from -1, 1 to 0, 1
    obo.diffuse_color = vec4_create(s, s, s, 1.0f);

    // load the data into the buffer
    vulkan_buffer_load_data(context, &shader->object_uniform_buffer, offset, range, 0, &obo);

    // only do this if the descriptor has not yet been updated - check to see if it has a valid id
    if (object_state->descriptor_states[descriptor_index].generations[image_index] == INVALID_ID) {
        VkDescriptorBufferInfo buffer_info;                         // define some buffer info
        buffer_info.buffer = shader->object_uniform_buffer.handle;  // pass it the handle to the object uniform buffer
        buffer_info.offset = offset;                                // the offset
        buffer_info.range = range;                                  // and the range

        // create the vulkan struct for creating a descriptor set, use the macro to format and fill with default values
        VkWriteDescriptorSet descriptor = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        descriptor.dstSet = object_descriptor_set;                      // destination is the object descriptor set
        descriptor.dstBinding = descriptor_index;                       // bind at the descriptor index
        descriptor.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;  // is a uniform buffer
        descriptor.descriptorCount = 1;                                 // only one for now
        descriptor.pBufferInfo = &buffer_info;                          // pass in the info from above

        descriptor_writes[descriptor_count] = descriptor;  // pass the struct to the descriptor writes array
        descriptor_count++;                                // increment the count of the array

        // update the frame generation. in this case it is only needed once since this is a buffer
        object_state->descriptor_states[descriptor_index].generations[image_index] = 1;
    }
    descriptor_index++;  // increment the descriptor index

    // TODO:  samplers - only one for now, but still going to setup the loop for more than one
    const u32 sampler_count = 1;
    VkDescriptorImageInfo image_infos[1];
    for (u32 sampler_index = 0; sampler_index < sampler_count; ++sampler_index) {                                  // iterate through all of the samplers
        texture* t = data.textures[sampler_index];                                                                 // align the textures with the samplers
        u32* descriptor_generation = &object_state->descriptor_states[descriptor_index].generations[image_index];  // get a pointer to the descriptor generation at image index

        // if the texture hasn't been loaded yet, use the default
        //  TODO: determine which use the texture has and pull appropriate default based on that
        if (t->generation == INVALID_ID) {
            t = shader->default_diffuse;

            // reset the descriptor generation if using the default texture
            *descriptor_generation = INVALID_ID;
        }

        // check if the descriptor needs updating first
        if (t && (*descriptor_generation != t->generation || *descriptor_generation == INVALID_ID)) {
            vulkan_texture_data* internal_data = (vulkan_texture_data*)t->internal_data;  // convenience

            // assign view and sampler
            image_infos[sampler_index].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            image_infos[sampler_index].imageView = internal_data->image.view;
            image_infos[sampler_index].sampler = internal_data->sampler;

            VkWriteDescriptorSet descriptor = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
            descriptor.dstSet = object_descriptor_set;
            descriptor.dstBinding = descriptor_index;
            descriptor.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptor.descriptorCount = 1;
            descriptor.pImageInfo = &image_infos[sampler_index];

            descriptor_writes[descriptor_count] = descriptor;
            descriptor_count++;

            // sync frame generation if not using a default texture
            if (t->generation != INVALID_ID) {
                *descriptor_generation = t->generation;
            }
            descriptor_index++;
        }
    }

    if (descriptor_count > 0) {                                                                             // if there are any
        vkUpdateDescriptorSets(context->device.logical_device, descriptor_count, descriptor_writes, 0, 0);  // update descriptor sets, with the new data
    }

    // bind the descriptor set to be updated, or in case the shader changed
    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shader->pipeline.pipeline_layout, 1, 1, &object_descriptor_set, 0, 0);
}

// pull in resources from outside the engine like meshes, and textures and such, and tag it with an id, to keep track of them without having to keep pointers
b8 vulkan_object_shader_acquire_resources(vulkan_context* context, struct vulkan_object_shader* shader, u32* out_object_id) {
    // TODO: free list
    *out_object_id = shader->object_uniform_buffer_index;  // derenferenc the out abject id and store the buffer index value there
    shader->object_uniform_buffer_index++;                 // increment the buffer index

    u32 object_id = *out_object_id;                                                       // define object id, give it the value of out_object_id
    vulkan_object_shader_object_state* object_state = &shader->object_states[object_id];  // get a pointer to the objects state
    for (u32 i = 0; i < VULKAN_OBJECT_SHADER_DESCRIPTOR_COUNT; ++i) {                     // iterate through the descriptors
        for (u32 j = 0; j < 3; ++j) {                                                     // set all three generations to invalid id
            object_state->descriptor_states[i].generations[j] = INVALID_ID;
        }
    }

    // allocate the descriptor sets
    VkDescriptorSetLayout layouts[3] = {
        shader->object_descriptor_set_layout,
        shader->object_descriptor_set_layout,
        shader->object_descriptor_set_layout};

    // create the vulkan struct for creating a descriptor set allocation, use the macro to format and fill with default values
    VkDescriptorSetAllocateInfo alloc_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    alloc_info.descriptorPool = shader->object_descriptor_pool;
    alloc_info.descriptorSetCount = 3;  // one per frame
    alloc_info.pSetLayouts = layouts;
    // allocate the descriptor sets, pass in the logical device, the info just created, and the descriptor sets to allocate
    VkResult result = vkAllocateDescriptorSets(context->device.logical_device, &alloc_info, object_state->descriptor_sets);
    if (result != VK_SUCCESS) {
        KERROR("Error allocating descriptor sets in shader!");
        return false;
    }

    return true;
}

void vulkan_object_shader_release_resources(vulkan_context* context, struct vulkan_object_shader* shader, u32 object_id) {
    vulkan_object_shader_object_state* object_state = &shader->object_states[object_id];  // get a pointer to the objects state

    const u32 descriptor_set_count = 3;
    // release object descriptor sets
    VkResult result = vkFreeDescriptorSets(context->device.logical_device, shader->object_descriptor_pool, descriptor_set_count, object_state->descriptor_sets);
    if (result != VK_SUCCESS) {
        KERROR("Error freeing object shader description sets!");
    }
    // re invalidate all of the descriptor generations
    for (u32 i = 0; i < VULKAN_OBJECT_SHADER_DESCRIPTOR_COUNT; ++i) {  // iterate through the descriptors
        for (u32 j = 0; j < 3; ++j) {                                  // set all three generations to invalid id
            object_state->descriptor_states[i].generations[j] = INVALID_ID;
        }
    }

    // TODO: the object id to the free list
}