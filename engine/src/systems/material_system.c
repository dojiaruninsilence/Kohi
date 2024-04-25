#include "material_system.h"

#include "core/logger.h"
#include "core/kstring.h"
#include "containers/hashtable.h"
#include "math/kmath.h"
#include "renderer/renderer_frontend.h"
#include "systems/texture_system.h"

#include "systems/resource_system.h"
#include "systems/shader_system.h"

typedef struct material_shader_uniform_locations {
    u16 projection;
    u16 view;
    u16 ambient_colour;
    u16 view_position;
    u16 shininess;
    u16 diffuse_colour;
    u16 diffuse_texture;
    u16 specular_texture;
    u16 normal_texture;
    u16 model;
    u16 render_mode;
} material_shader_uniform_locations;

typedef struct ui_shader_uniform_locations {
    u16 projection;
    u16 view;
    u16 diffuse_colour;
    u16 diffuse_texture;
    u16 model;
} ui_shader_uniform_locations;

// store the material system state here
typedef struct material_system_state {
    material_system_config config;

    material default_material;

    // an array of registered materials
    material* registered_materials;

    // hashtable for material lookups
    hashtable registered_material_table;

    // known locations for the material shader
    material_shader_uniform_locations material_locations;
    u32 material_shader_id;

    // known locations for the ui shader
    ui_shader_uniform_locations ui_locations;
    u32 ui_shader_id;
} material_system_state;

// hold the data for referencing materials
typedef struct material_reference {
    u64 reference_count;  // hold the count of how many times the material is referenced
    u32 handle;           // the index into the array of registered materials
    b8 auto_release;      // is the material auto release
} material_reference;

static material_system_state* state_ptr = 0;  // get a pointer to the system state

// forward declaration private functions
b8 create_default_material(material_system_state* state);
b8 load_material(material_config config, material* m);
void destroy_material(material* m);

// initialize the material system -- this is a 2 step function, call the first time without the pointer to the block of memory
// it will return the amount of memory the system will require to run, memory must then be allocated,
// and then passed in to the function and ran again, this time actually initializing the system
b8 material_system_initialize(u64* memory_requirement, void* state, material_system_config config) {
    if (config.max_material_count == 0) {  // cant have a material system with no materials
        KFATAL("material_system_initialize - config.max_material_count must be > 0.");
        return false;
    }

    // block of memory will contain state structure, them block for array, the block for the hashtable
    u64 struct_requirement = sizeof(material_system_state);
    u64 array_requirement = sizeof(material) * config.max_material_count;
    u64 hashtable_requirement = sizeof(material_reference) * config.max_material_count;
    *memory_requirement = struct_requirement + array_requirement + hashtable_requirement;

    // here is where we boot out if its the first pass and all we need is the memory requirements
    if (!state) {
        return true;
    }

    // pass in the state pointer and the config data
    state_ptr = state;
    state_ptr->config = config;

    state_ptr->material_shader_id = INVALID_ID;
    state_ptr->material_locations.view = INVALID_ID_U16;
    state_ptr->material_locations.projection = INVALID_ID_U16;
    state_ptr->material_locations.diffuse_colour = INVALID_ID_U16;
    state_ptr->material_locations.diffuse_texture = INVALID_ID_U16;
    state_ptr->material_locations.specular_texture = INVALID_ID_U16;
    state_ptr->material_locations.normal_texture = INVALID_ID_U16;
    state_ptr->material_locations.ambient_colour = INVALID_ID_U16;
    state_ptr->material_locations.shininess = INVALID_ID_U16;
    state_ptr->material_locations.model = INVALID_ID_U16;
    state_ptr->material_locations.render_mode = INVALID_ID_U16;

    state_ptr->ui_shader_id = INVALID_ID;
    state_ptr->ui_locations.diffuse_colour = INVALID_ID_U16;
    state_ptr->ui_locations.diffuse_texture = INVALID_ID_U16;
    state_ptr->ui_locations.view = INVALID_ID_U16;
    state_ptr->ui_locations.projection = INVALID_ID_U16;
    state_ptr->ui_locations.model = INVALID_ID_U16;

    // the array block is after the state. already allocated, so just set the pointer
    void* array_block = state + struct_requirement;
    state_ptr->registered_materials = array_block;

    // hashtable block is after the array
    void* hashtable_block = array_block + array_requirement;

    // create a hashtable for material lookups
    hashtable_create(sizeof(material_reference), config.max_material_count, hashtable_block, false, &state_ptr->registered_material_table);

    // fill the hashtable with invalid references to use as a default
    material_reference invalid_ref;
    invalid_ref.auto_release = false;
    invalid_ref.handle = INVALID_ID;  // primary reason for needing the default values
    invalid_ref.reference_count = 0;
    hashtable_fill(&state_ptr->registered_material_table, &invalid_ref);

    // invalidate all the materials in the array
    u32 count = state_ptr->config.max_material_count;
    for (u32 i = 0; i < count; i++) {
        state_ptr->registered_materials[i].id = INVALID_ID;
        state_ptr->registered_materials[i].generation = INVALID_ID;
        state_ptr->registered_materials[i].internal_id = INVALID_ID;
        state_ptr->registered_materials[i].render_frame_number = INVALID_ID;
    }

    // create the default material, this is required, if it fails crash
    if (!create_default_material(state_ptr)) {
        KFATAL("Failed to create default material. Application cannot continue");
        return false;
    }

    return true;
}

// shut down the material system
void material_system_shutdown(void* state) {
    material_system_state* s = (material_system_state*)state;
    if (s) {
        // invalidate all materials in the array
        u32 count = s->config.max_material_count;
        for (u32 i = 0; i < count; ++i) {                       // iterate through all of the materials
            if (s->registered_materials[i].id != INVALID_ID) {  // if any are still registered
                destroy_material(&s->registered_materials[i]);  // destroy them
            }
        }

        // destroy the default material
        destroy_material(&s->default_material);
    }

    // reset the state pointer
    state_ptr = 0;
}

// acquire a material by name from file - assumes that there is a material with that name
material* material_system_acquire(const char* name) {
    // load the given material configuration from resource
    resource material_resource;
    if (!resource_system_load(name, RESOURCE_TYPE_MATERIAL, &material_resource)) {
        KERROR("Failed to load material resource, returning nullptr.");
        return 0;
    }

    // now acquire from loaded config
    material* m = 0;
    if (material_resource.data) {
        m = material_system_acquire_from_config(*(material_config*)material_resource.data);
    }

    // clean up
    resource_system_unload(&material_resource);

    if (!m) {
        KERROR("Failed to load material resource, returning nullptr.");
    }

    return m;
}

// acquire a material system from a config - from code instead of a file
material* material_system_acquire_from_config(material_config config) {
    // return default material
    if (strings_equali(config.name, DEFAULT_MATERIAL_NAME)) {
        return &state_ptr->default_material;
    }

    // get a copy of the hash table and store it in ref
    material_reference ref;
    if (state_ptr && hashtable_get(&state_ptr->registered_material_table, config.name, &ref)) {  // if the copy fails, or the state pointer is null
        // this can only be changed the fist time a material is loaded
        if (ref.reference_count == 0) {
            ref.auto_release = config.auto_release;
        }
        ref.reference_count++;  // increment the reference count
        if (ref.handle == INVALID_ID) {
            // this means no material exists here. find a free index first
            u32 count = state_ptr->config.max_material_count;  // store the max material count in count for ease
            material* m = 0;                                   // define a material pointer
            for (u32 i = 0; i < count; ++i) {                  // iterate through the entire material array looking for an empty slot
                if (state_ptr->registered_materials[i].id == INVALID_ID) {
                    // a free slot has been found. use its index as the handle
                    ref.handle = i;
                    m = &state_ptr->registered_materials[i];
                    break;
                }
            }

            // make sure that an empty slot was actually found
            if (!m || ref.handle == INVALID_ID) {
                KFATAL("material_system_acquire - Material system cannot hold anymore materials. Adjust configuration to allow more.");
                return 0;
            }

            // create a new material
            if (!load_material(config, m)) {
                KERROR("Failed to load material '%s'.", config.name);
                return 0;
            }

            // get the uniform indices
            shader* s = shader_system_get_by_id(m->shader_id);
            // save off the locations for known types for quick lookups
            if (state_ptr->material_shader_id == INVALID_ID && strings_equal(config.shader_name, BUILTIN_SHADER_NAME_MATERIAL)) {
                state_ptr->material_shader_id = s->id;
                state_ptr->material_locations.projection = shader_system_uniform_index(s, "projection");
                state_ptr->material_locations.view = shader_system_uniform_index(s, "view");
                state_ptr->material_locations.ambient_colour = shader_system_uniform_index(s, "ambient_colour");
                state_ptr->material_locations.view_position = shader_system_uniform_index(s, "view_position");
                state_ptr->material_locations.diffuse_colour = shader_system_uniform_index(s, "diffuse_colour");
                state_ptr->material_locations.diffuse_texture = shader_system_uniform_index(s, "diffuse_texture");
                state_ptr->material_locations.specular_texture = shader_system_uniform_index(s, "specular_texture");
                state_ptr->material_locations.normal_texture = shader_system_uniform_index(s, "normal_texture");
                state_ptr->material_locations.shininess = shader_system_uniform_index(s, "shininess");
                state_ptr->material_locations.model = shader_system_uniform_index(s, "model");
                state_ptr->material_locations.render_mode = shader_system_uniform_index(s, "mode");
            } else if (state_ptr->ui_shader_id == INVALID_ID && strings_equal(config.shader_name, BUILTIN_SHADER_NAME_UI)) {
                state_ptr->ui_shader_id = s->id;
                state_ptr->ui_locations.projection = shader_system_uniform_index(s, "projection");
                state_ptr->ui_locations.view = shader_system_uniform_index(s, "view");
                state_ptr->ui_locations.diffuse_colour = shader_system_uniform_index(s, "diffuse_colour");
                state_ptr->ui_locations.diffuse_texture = shader_system_uniform_index(s, "diffuse_texture");
                state_ptr->ui_locations.model = shader_system_uniform_index(s, "model");
            }

            // if this is the first time loading this texture set generation to 0, if not increment it
            if (m->generation == INVALID_ID) {
                m->generation = 0;
            } else {
                m->generation++;
            }

            // also use the handle as the material id
            m->id = ref.handle;
            // KTRACE("Material '%s' does not yet exist. Created, and ref_count is now %i.", config.name, ref.reference_count);
        } else {
            // KTRACE("Material '%s' already exists. ref_count increased to %i.", config.name, ref.reference_count);
        }

        // update the entry
        hashtable_set(&state_ptr->registered_material_table, config.name, &ref);
        return &state_ptr->registered_materials[ref.handle];
    }

    // NOTE: this would only happen in the event something went wrong with the state
    KERROR("material_system_acquire_from_config failed to acquire material '%s'. Null pointer will be returned.", config.name);
    return 0;
}

// release a material by name
void material_system_release(const char* name) {
    // ignore release requests for the default material
    if (strings_equali(name, DEFAULT_MATERIAL_NAME)) {
        return;
    }
    material_reference ref;
    if (state_ptr && hashtable_get(&state_ptr->registered_material_table, name, &ref)) {
        if (ref.reference_count == 0) {
            KWARN("Tried to release non-existant material: '%s'", name);
            return;
        }
        ref.reference_count--;
        if (ref.reference_count == 0 && ref.auto_release) {
            material* m = &state_ptr->registered_materials[ref.handle];

            // destroy/reset material
            destroy_material(m);

            // reset the reference
            ref.handle = INVALID_ID;
            ref.auto_release = false;
            // KTRACE("Released material '%s'.  Material unloaded because reference count = 0 and auto release = true.", name);
        } else {
            // KTRACE("Released material '%s'.  Now has a reference count = '%i' and auto release = %s.", name, ref.reference_count, ref.auto_release ? "true" : "false");
        }

        // update the entry
        hashtable_set(&state_ptr->registered_material_table, name, &ref);
    } else {
        KERROR("material_system_release failed to release material '%s'.", name);
    }
}

// get the default material
material* material_system_get_default() {
    if (state_ptr) {
        return &state_ptr->default_material;
    }

    KFATAL("material_system_get_default called before the system initialized");
    return 0;
}

#define MATERIAL_APPLY_OR_FAIL(expr)                  \
    if (!expr) {                                      \
        KERROR("Failed to apply material: %s", expr); \
        return false;                                 \
    }

// @brief applies global-level data for the material shader id.
// @param shader_id the identifier of the shader to apply globals for
// @param projection a constant pointer to a projection matrix.
// @param view a constant pointer to a view matrix
// @return true on success, otherwise false
b8 material_system_apply_global(u32 shader_id, const mat4* projection, const mat4* view, const vec4* ambient_colour, const vec3* view_posistion, u32 render_mode) {
    if (shader_id == state_ptr->material_shader_id) {
        MATERIAL_APPLY_OR_FAIL(shader_system_uniform_set_by_index(state_ptr->material_locations.projection, projection));
        MATERIAL_APPLY_OR_FAIL(shader_system_uniform_set_by_index(state_ptr->material_locations.view, view));
        MATERIAL_APPLY_OR_FAIL(shader_system_uniform_set_by_index(state_ptr->material_locations.ambient_colour, ambient_colour));
        MATERIAL_APPLY_OR_FAIL(shader_system_uniform_set_by_index(state_ptr->material_locations.view_position, view_posistion));
        MATERIAL_APPLY_OR_FAIL(shader_system_uniform_set_by_index(state_ptr->material_locations.render_mode, &render_mode));
    } else if (shader_id == state_ptr->ui_shader_id) {
        MATERIAL_APPLY_OR_FAIL(shader_system_uniform_set_by_index(state_ptr->ui_locations.projection, projection));
        MATERIAL_APPLY_OR_FAIL(shader_system_uniform_set_by_index(state_ptr->ui_locations.view, view));
    } else {
        KERROR("material_system_apply_global(): Unrecognized shader id '%d' ", shader_id);
        return false;
    }
    MATERIAL_APPLY_OR_FAIL(shader_system_apply_global());
    return true;
}

// @brief applies instance level material data for the given material
// @param m a pointer to the material to be applied
// @return true on success; otherwise false
b8 material_system_apply_instance(material* m, b8 needs_update) {
    // apply instance level uniforms
    MATERIAL_APPLY_OR_FAIL(shader_system_bind_instance(m->internal_id));
    if (needs_update) {
        if (m->shader_id == state_ptr->material_shader_id) {
            // Material shader
            MATERIAL_APPLY_OR_FAIL(shader_system_uniform_set_by_index(state_ptr->material_locations.diffuse_colour, &m->diffuse_colour));
            MATERIAL_APPLY_OR_FAIL(shader_system_uniform_set_by_index(state_ptr->material_locations.diffuse_texture, &m->diffuse_map));
            MATERIAL_APPLY_OR_FAIL(shader_system_uniform_set_by_index(state_ptr->material_locations.specular_texture, &m->specular_map));
            MATERIAL_APPLY_OR_FAIL(shader_system_uniform_set_by_index(state_ptr->material_locations.normal_texture, &m->normal_map));
            MATERIAL_APPLY_OR_FAIL(shader_system_uniform_set_by_index(state_ptr->material_locations.shininess, &m->shininess));
        } else if (m->shader_id == state_ptr->ui_shader_id) {
            // UI shader
            MATERIAL_APPLY_OR_FAIL(shader_system_uniform_set_by_index(state_ptr->ui_locations.diffuse_colour, &m->diffuse_colour));
            MATERIAL_APPLY_OR_FAIL(shader_system_uniform_set_by_index(state_ptr->ui_locations.diffuse_texture, &m->diffuse_map));
        } else {
            KERROR("material_system_apply_instance(): Unrecognized shader id '%d' on shader '%s'.", m->shader_id, m->name);
            return false;
        }
    }
    MATERIAL_APPLY_OR_FAIL(shader_system_apply_instance(needs_update));

    return true;
}

// @brief applies local level material data (typically just a model matrix).
// @param m a pointer to the material to be applied
// @param model a constant pointer to the model matrix to be applied
// @return true on success, otherwise false
b8 material_system_apply_local(material* m, const mat4* model) {
    if (m->shader_id == state_ptr->material_shader_id) {
        return shader_system_uniform_set_by_index(state_ptr->material_locations.model, model);
    } else if (m->shader_id == state_ptr->ui_shader_id) {
        return shader_system_uniform_set_by_index(state_ptr->ui_locations.model, model);
    }

    KERROR("Unrecognized shader id '%d'", m->shader_id);
    return false;
}

b8 load_material(material_config config, material* m) {
    kzero_memory(m, sizeof(material));

    // name
    string_ncopy(m->name, config.name, MATERIAL_NAME_MAX_LENGTH);

    m->shader_id = shader_system_get_id(config.shader_name);

    // diffuse color
    m->diffuse_colour = config.diffuse_colour;
    m->shininess = config.shininess;

    // diffuse map
    // TODO: make this configurable
    // TODO: dry
    m->diffuse_map.filter_minify = m->diffuse_map.filter_magnify = TEXTURE_FILTER_MODE_LINEAR;
    m->diffuse_map.repeat_u = m->diffuse_map.repeat_v = m->diffuse_map.repeat_w = TEXTURE_REPEAT_REPEAT;
    if (!renderer_texture_map_acquire_resources(&m->diffuse_map)) {
        KERROR("Unable to acquire resources for diffuse texture map.");
        return false;
    }
    if (string_length(config.diffuse_map_name) > 0) {
        m->diffuse_map.use = TEXTURE_USE_MAP_DIFFUSE;
        m->diffuse_map.texture = texture_system_acquire(config.diffuse_map_name, true);
        if (!m->diffuse_map.texture) {
            // configured but not found
            KWARN("Unable to load texture '%s' for material '%s', using default.", config.diffuse_map_name, m->name);
            m->diffuse_map.texture = texture_system_get_default_texture();
        }
    } else {
        // this is done when a texture is not configured, as opposed to when it is configured and not found (above)
        m->diffuse_map.use = TEXTURE_USE_MAP_DIFFUSE;
        m->diffuse_map.texture = texture_system_get_default_diffuse_texture();
    }

    // specular map
    // TODO: make this configurable
    m->specular_map.filter_minify = m->specular_map.filter_magnify = TEXTURE_FILTER_MODE_LINEAR;
    m->specular_map.repeat_u = m->specular_map.repeat_v = m->specular_map.repeat_w = TEXTURE_REPEAT_REPEAT;
    if (!renderer_texture_map_acquire_resources(&m->specular_map)) {
        KERROR("Unable to acquire resources for specular texture map.");
        return false;
    }
    if (string_length(config.specular_map_name) > 0) {
        m->specular_map.use = TEXTURE_USE_MAP_SPECULAR;
        m->specular_map.texture = texture_system_acquire(config.specular_map_name, true);
        if (!m->specular_map.texture) {
            KWARN("Unable to load specular texture '%s' for material '%s', using default", config.specular_map_name, m->name);
            m->specular_map.texture = texture_system_get_default_specular_texture();
        }
    } else {
        // NOTE: only set for clarity, as call to kzero_memory above does this already
        m->specular_map.use = TEXTURE_USE_MAP_SPECULAR;
        m->specular_map.texture = texture_system_get_default_specular_texture();
    }

    // normal map
    // TODO: make this configurable
    m->normal_map.filter_minify = m->normal_map.filter_magnify = TEXTURE_FILTER_MODE_LINEAR;
    m->normal_map.repeat_u = m->normal_map.repeat_v = m->normal_map.repeat_w = TEXTURE_REPEAT_REPEAT;
    if (!renderer_texture_map_acquire_resources(&m->normal_map)) {
        KERROR("Unable to acquire resources for normal texture map.");
        return false;
    }
    if (string_length(config.normal_map_name) > 0) {
        m->normal_map.use = TEXTURE_USE_MAP_NORMAL;
        m->normal_map.texture = texture_system_acquire(config.normal_map_name, true);
        if (!m->normal_map.texture) {
            KWARN("Unable to load normal texture '%s' for material '%s', using default.", config.normal_map_name, m->name);
            m->normal_map.texture = texture_system_get_default_normal_texture();
        }
    } else {
        // use default
        m->normal_map.use = TEXTURE_USE_MAP_NORMAL;
        m->normal_map.texture = texture_system_get_default_normal_texture();
    }

    // TODO: other maps

    // sent it off to the renderer to acquire resources
    shader* s = shader_system_get(config.shader_name);
    if (!s) {
        KERROR("Unable to load material because its shader was not found: '%s'. This is likely a problem with the material asset.", config.shader_name);
        return false;
    }
    // gather a list of pointers to texture maps
    texture_map* maps[3] = {&m->diffuse_map, &m->specular_map, &m->normal_map};
    if (!renderer_shader_acquire_instance_resources(s, maps, &m->internal_id)) {
        KERROR("Failed to acquire renderer resources for material '%s'.", m->name);
        return false;
    }

    return true;
}

void destroy_material(material* m) {
    // KTRACE("Destroying material '%s'...", m->name);

    // release texture references
    if (m->diffuse_map.texture) {
        texture_system_release(m->diffuse_map.texture->name);
    }
    if (m->specular_map.texture) {
        texture_system_release(m->specular_map.texture->name);
    }
    if (m->normal_map.texture) {
        texture_system_release(m->normal_map.texture->name);
    }

    // release texture map resources
    renderer_texture_map_release_resources(&m->diffuse_map);
    renderer_texture_map_release_resources(&m->specular_map);
    renderer_texture_map_release_resources(&m->normal_map);

    // release renderer resources
    if (m->shader_id != INVALID_ID && m->internal_id != INVALID_ID) {
        renderer_shader_release_instance_resources(shader_system_get_by_id(m->shader_id), m->internal_id);
        m->shader_id = INVALID_ID;
    }

    // zero it out, invalidate IDs
    kzero_memory(m, sizeof(material));
    m->id = INVALID_ID;
    m->generation = INVALID_ID;
    m->internal_id = INVALID_ID;
    m->render_frame_number = INVALID_ID;
}

b8 create_default_material(material_system_state* state) {
    kzero_memory(&state->default_material, sizeof(material));
    state->default_material.id = INVALID_ID;
    state->default_material.generation = INVALID_ID;
    string_ncopy(state->default_material.name, DEFAULT_MATERIAL_NAME, MATERIAL_NAME_MAX_LENGTH);
    state->default_material.diffuse_colour = vec4_one();  // white
    state->default_material.diffuse_map.use = TEXTURE_USE_MAP_DIFFUSE;
    state->default_material.diffuse_map.texture = texture_system_get_default_texture();

    state->default_material.specular_map.use = TEXTURE_USE_MAP_SPECULAR;
    state->default_material.specular_map.texture = texture_system_get_default_specular_texture();

    state->default_material.normal_map.use = TEXTURE_USE_MAP_NORMAL;
    state->default_material.normal_map.texture = texture_system_get_default_normal_texture();

    texture_map* maps[3] = {&state->default_material.diffuse_map, &state->default_material.specular_map, &state->default_material.normal_map};

    shader* s = shader_system_get(BUILTIN_SHADER_NAME_MATERIAL);
    if (!renderer_shader_acquire_instance_resources(s, maps, &state->default_material.internal_id)) {
        KFATAL("Failed to aquire renderer resources for default material. Application cannot continue.");
        return false;
    }

    // make sure to assign the shader id
    state->default_material.shader_id = s->id;

    return true;
}