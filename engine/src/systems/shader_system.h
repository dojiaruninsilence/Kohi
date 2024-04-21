#pragma once

#include "defines.h"
#include "renderer/renderer_types.inl"
#include "containers/hashtable.h"

// @brief configuration for the shader system
typedef struct shader_system_config {
    // @brief the maximum number of shaders held in the system. NOTE: should be at least 512
    u16 max_shader_count;
    // @brief the maximum number of uniforms allowed in a single shader
    u8 max_uniform_count;
    // @brief the maximum number of global-scope textures allowed in a single shader.
    u8 max_global_textures;
    // @brief the maximum number of instance-scope textures allowed in a single shader
    u8 max_instance_textures;
} shader_system_config;

// @brief represents the current state of a given shader
typedef enum shader_state {
    // @brief the shader has not yet gone through the creation process, and is unusable
    SHADER_STATE_NOT_CREATED,
    // @brief the shader has gone through the creation process, but not initialization. it is unusable
    SHADER_STATE_UNINITIALIZED,
    // @brief the shader is created and initialized, and is ready for use
    SHADER_STATE_INITIALIZED,
} shader_state;

// @brief represents a single entry in the internal uniform array.
typedef struct shader_uniform {
    // @brief the offset in bytes from the beginning of the uniform set (global/instance/local)
    u64 offset;
    // @brief the location to be used as a lookup. typically the same as the index except for samplers,
    // which is used to lookup texture index within the internal array at the given scope (global/instance)
    u16 location;
    // @brief index into the internal uniform array
    u16 index;
    // @brief the size of the uniform, or 0 for samplers
    u16 size;
    // @brief the index of the descriptor set the uniform belongs to (0=global, 1=instance, INVALID_ID=local)
    u8 set_index;
    // @brief the scope of the uniform
    shader_scope scope;
    // @brief the type of uniform
    shader_uniform_type type;
} shader_uniform;

// @brief represents a single shader vertex attribute
typedef struct shader_attribute {
    // @brief the attribute name
    char* name;
    // @brief the attribute type
    shader_attribute_type type;
    // @brief the attribute size in bytes
    u32 size;
} shader_attribute;

// @brief represents a shader on the frontend
typedef struct shader {
    // @brief the shader identifier
    u32 id;

    char* name;

    // @brief indicates if the shader uses instances. if not, it is assumed
    // that only global uniforms and samplers are used
    b8 use_instances;
    // @brief indicates if locals are used (typically for model matrices, ect.)
    b8 use_locals;

    // @brief the amount of bytes that are required for ubo anlignment.
    // this is used along with the ubo size to determine the ultimate stride, which is how much the ubos are spaced
    // out in the buffer.  for example, a required alignment of 256 means that the stride must be a multiple of 256
    // (true for some nvidia cards like mine, dammit)
    u64 required_ubo_alignment;

    // @brief the actual size of the global uniform buffer object
    u64 global_ubo_size;
    // @brief the stride of the global uniform buffer object
    u64 global_ubo_stride;
    // @brief the offset in bytes for the global ubo from the beginning of the uniform buffer
    u64 global_ubo_offset;

    // @brief the actual size of the instance uniform buffer object
    u64 ubo_size;

    // @brief the stride of the instance uniform buffer object
    u64 ubo_stride;

    // @brief the total size of all push constant ranges combined
    u64 push_constant_size;
    // @brief the push constant stride, aligned to 4 bytes as required by vulkan
    u64 push_constant_stride;

    // @brief an array of global texture map pointers. darray
    texture_map** global_texture_maps;

    // @brief the number of instance textures
    u8 instance_texture_count;

    shader_scope bound_scope;

    // @brief the identifier of the currently bound instance
    u32 bound_instance_id;
    // @brief the currently bound instance's ubo offset
    u32 bound_ubo_offset;

    // @brief the block of memory used by the uniform hashtable
    void* hashtable_block;
    // @brief a hashtable to stor uniform index/locations by name
    hashtable uniform_lookup;

    // @brief an array of uniforms in this shader. darray.
    shader_uniform* uniforms;

    // @brief an array of attributes. darray
    shader_attribute* attributes;

    // @brief the internal state of the shader
    shader_state state;

    // @brief the number of push constant ranges
    u8 push_constant_range_count;
    // @brief an array of push constant ranges
    range push_constant_ranges[32];
    // @brief the size of all attributes combined, aka the size of the vertex
    u16 attribute_stride;

    // @brief an opaque pointer to hold renderer api specific data. renderer is responsible for creation and destruction of this
    void* internal_data;
} shader;

// @brief initializes the shader system using the supplied configuration
// NOTE: call this twice, once to obtain the memory requirements(memory=0) and the second time including allocated memory to actually initialize
// @param memory_requirement a pointer to hold the memory requirement of this system in bytes
// @param memory a memory block to be used to hold the state of this system. pass 0 on the first call to get memory requirement
// @param config the configuration to be used when initializing the system
// @return b8 true on success, otherwise false
b8 shader_system_initialize(u64* memory_requirement, void* memory, shader_system_config config);

// @brief shuts down the shader system
// @param state a pointer to the system state
void shader_system_shutdown(void* state);

// @brief creates a new shader with the given config
// @param config the configuration to be used when creating the shader
// @return true on success, otherwise false
KAPI b8 shader_system_create(const shader_config* config);

// @brief gets the identifier of a shader by name
// @param shader_name the name of the shader
// @return the shader id, if found; otherwise invalid id
KAPI u32 shader_system_get_id(const char* shader_name);

// @brief returns a pointer to a shader with the given identifier
// @param shader_id the shader identifier
// @return a pointer to a shader, if found, otherwise 0
KAPI shader* shader_system_get_by_id(u32 shader_id);

// @brief returns a pointer to a shader with the given name
// @param shader_name the name to search for. case sensitive
// @return a pointer to a shader, if found; otherwise 0
KAPI shader* shader_system_get(const char* shader_name);

// @brief uses the shader with the given name
// @param shader_name the name of the shader to use. case sensitive
// @return true on success, otherwise false
KAPI b8 shader_system_use(const char* shader_name);

// @brief uses the shader with the given identifier
// @param shader_id the identifier of the shader to be used
// @return true on success, otherwise false
KAPI b8 shader_system_use_by_id(u32 shader_id);

// @brief returns the uniform index for a uniform with the given name, if found
// @param a pointer to the shader to obtain the index from
// @param uniform_name the name of the uniform to search for
// @return the uniform index, if found, otherwise INVALID_ID_U16
KAPI u16 shader_system_uniform_index(shader* s, const char* uniform_name);

// @brief sets the value of a uniform with the given name to the supplied value
// NOTE: operates against the currently used shader
// @param uniform-name the name of the uniform to be set
// @param value the value to be set
// @return true if success, otherwise false
KAPI b8 shader_system_uniform_set(const char* uniform_name, const void* value);

// @brief sets the texture of a sampler with the given name to the supplied texture
// NOTE: operates against the currently-used shader
// @param uniform_name the name of the uniform to be set
// @param a pointer to the texture to be set
// @return true on success, otherwise false
KAPI b8 shader_system_sampler_set(const char* sampler_name, const texture* t);

// @brief sets a uniform value by index
// NOTE: operates against the currently-used shader
// @param index the index of the uniform
// @param value the value of the uniform
// @return true on success, otherwise false
KAPI b8 shader_system_uniform_set_by_index(u16 index, const void* value);

// @brief sets a sampler value by index
// NOTE: operates against the currently-used shader
// @param index the index of the uniform
// @param value a pointer to the texture to be set
// @return true on success, otherwise false
KAPI b8 shader_system_sampler_set_by_index(u16 index, const struct texture* t);

// @brief applies global scoped uniforms
// NOTE: operates against the currently-used shader
// @return true on success, otherwise false
KAPI b8 shader_system_apply_global();

// @brief applies instance scoped uniforms
// NOTE: operates against the currently-used shader
// @param needs_update indicates if shader internals need to be updated, or just to be bound.
// @return true on success, otherwise false
KAPI b8 shader_system_apply_instance(b8 needs_update);

// @brief binds the instance with the given id for use. must be done before setting instance scoped uniforms
// NOTE: operates against the currently-used shader
// @param instance_id the identifier of the instance to bind
// @return true on success, otherwise false
KAPI b8 shader_system_bind_instance(u32 instance_id);
