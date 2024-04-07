#include "material_system.h"

#include "core/logger.h"
#include "core/kstring.h"
#include "containers/hashtable.h"
#include "math/kmath.h"
#include "renderer/renderer_frontend.h"
#include "systems/texture_system.h"

// TODO: temp: resource system
#include "platform/filesystem.h"
// end temp

// store the material system state here
typedef struct material_system_state {
    material_system_config config;

    material default_material;

    // an array of registered materials
    material* registered_materials;

    // hashtable for material lookups
    hashtable registered_material_table;
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
b8 load_configuration_file(const char* path, material_config* out_config);

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
    // load the given material configuration from disk
    material_config config;

    // load file from disk
    // TODO: should be able to ve located anywhere
    char* format_str = "assets/materials/%s.%s";
    char full_file_path[512];  // output buffer

    // TODO: try different extensions
    string_format(full_file_path, format_str, name, "kmt");
    if (!load_configuration_file(full_file_path, &config)) {
        KERROR("Failed to load material file: '%s'. Null pointer will be returned.", full_file_path);
        return 0;
    }

    // now acquire from loaded config
    return material_system_acquire_from_config(config);
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

            // if this is the first time loading this texture set generation to 0, if not increment it
            if (m->generation == INVALID_ID) {
                m->generation = 0;
            } else {
                m->generation++;
            }

            // also use the handle as the material id
            m->id = ref.handle;
            KTRACE("Material '%s' does not yet exist. Created, and ref_count is now %i.", config.name, ref.reference_count);
        } else {
            KTRACE("Material '%s' already exists. ref_count increased to %i.", config.name, ref.reference_count);
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
            KTRACE("Released material '%s'.  Material unloaded because reference count = 0 and auto release = true.", name);
        } else {
            KTRACE("Released material '%s'.  Now has a reference count = '%i' and auto release = %s.", name, ref.reference_count, ref.auto_release ? "true" : "false");
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

b8 load_material(material_config config, material* m) {
    kzero_memory(m, sizeof(material));

    // name
    string_ncopy(m->name, config.name, MATERIAL_NAME_MAX_LENGTH);

    // diffuse color
    m->diffuse_colour = config.diffuse_colour;

    // diffuse map
    if (string_length(config.diffuse_map_name) > 0) {
        m->diffuse_map.use = TEXTURE_USE_MAP_DIFFUSE;
        m->diffuse_map.texture = texture_system_aquire(config.diffuse_map_name, true);
        if (!m->diffuse_map.texture) {
            KWARN("Unable to load texture '%s' for material '%s', using default.", config.diffuse_map_name, m->name);
            m->diffuse_map.texture = texture_system_get_default_texture();
        }
    } else {
        // NOTE: only set for clarity, as call to kzero_memory above does this already
        m->diffuse_map.use = TEXTURE_USE_UNKNOWN;
        m->diffuse_map.texture = 0;
    }

    // TODO: other maps

    // sent it off to the renderer to acquire resources
    if (!renderer_create_material(m)) {
        KERROR("Failed to acquire renderer resources for material '%s'.", m->name);
        return false;
    }

    return true;
}

void destroy_material(material* m) {
    KTRACE("Destroying material '%s'...", m->name);

    // release renderer resources
    renderer_destroy_material(m);

    // zero it out, invalidate IDs
    kzero_memory(m, sizeof(material));
    m->id = INVALID_ID;
    m->generation = INVALID_ID;
    m->internal_id = INVALID_ID;
}

b8 create_default_material(material_system_state* state) {
    kzero_memory(&state->default_material, sizeof(material));
    state->default_material.id = INVALID_ID;
    state->default_material.generation = INVALID_ID;
    string_ncopy(state->default_material.name, DEFAULT_MATERIAL_NAME, MATERIAL_NAME_MAX_LENGTH);
    state->default_material.diffuse_colour = vec4_one();  // white
    state->default_material.diffuse_map.use = TEXTURE_USE_MAP_DIFFUSE;
    state->default_material.diffuse_map.texture = texture_system_get_default_texture();

    if (!renderer_create_material(&state->default_material)) {
        KFATAL("Failed to aquire renderer resources for default texture. Application cannot continue.");
        return false;
    }

    return true;
}

b8 load_configuration_file(const char* path, material_config* out_config) {
    file_handle f;
    if (!filesystem_open(path, FILE_MODE_READ, false, &f)) {
        KERROR("load_configuration_file - unable to open material file for reading: '%s'.", path);
        return false;
    }

    // read each line of the file
    char line_buf[512] = "";                                   // an empty char array 512 chars long
    char* p = &line_buf[0];                                    // get a pointer to the first char in the array
    u64 line_length = 0;                                       // the default, for reporting purposes
    u32 line_number = 1;                                       // the default, for reporting purposes
    while (filesystem_read_line(&f, 511, &p, &line_length)) {  // as long as there are valid lines to read, pass in 511 for the max length, so we dont go over the buffer
        // trim the string
        char* trimmed = string_trim(line_buf);

        // get the trimmed length
        line_length = string_length(trimmed);

        // skip blank lines and comments.
        if (line_length < 1 || trimmed[0] == '#') {  // if there is nothing left after trimming blank spaces, or the first char is #(indicates a commented line)
            line_number++;                           // increment the line number
            continue;                                // start the loop over?
        }

        // split into variables and values
        i32 equal_index = string_index_of(trimmed, '=');  // find the index number of the '=' char
        if (equal_index == -1) {                          // if no '=' was found
            KWARN("Potential formatting issue found in file '%s': '=' token not found. Skipping line %ui.", path, line_number);
            line_number++;  // increment the line number
            continue;       // start the loop over?
        }

        // assume a max of 64 characters for the variable name
        char raw_var_name[64];
        kzero_memory(raw_var_name, sizeof(char) * 64);       // just to be safe
        string_mid(raw_var_name, trimmed, 0, equal_index);   // copy a substring from index 0 to equal index
        char* trimmed_var_name = string_trim(raw_var_name);  // trim any space from the raw var, and save in trimmed var

        // assume a max of 511-65 (446) for the max length of the value to account for the variable name and the '='
        char raw_value[446];
        kzero_memory(raw_value, sizeof(char) * 446);          // just to be safe
        string_mid(raw_value, trimmed, equal_index + 1, -1);  // copy a substring from the equal index plus one all the way to the end of the line
        char* trimmed_value = string_trim(raw_value);         // trim the spaces off the ends of the string, and save under trimmed value

        // process the variable
        if (strings_equali(trimmed_var_name, "version")) {
            // TODO: version
        } else if (strings_equali(trimmed_var_name, "name")) {
            string_ncopy(out_config->name, trimmed_value, MATERIAL_NAME_MAX_LENGTH);
        } else if (strings_equali(trimmed_var_name, "diffuse_map_name")) {
            string_ncopy(out_config->diffuse_map_name, trimmed_value, TEXTURE_NAME_MAX_LENGTH);
        } else if (strings_equali(trimmed_var_name, "diffuse_colour")) {
            // parse the color
            if (!string_to_vec4(trimmed_value, &out_config->diffuse_colour)) {
                KWARN("Error parsing diffuse_colour in file '%s'. Using default of white instead.", path);
                out_config->diffuse_colour = vec4_one();  // white
            }
        }

        // TODO: more fields will be going here

        // clear the line buffer
        kzero_memory(line_buf, sizeof(char) * 512);
        line_number++;
    }

    filesystem_close(&f);

    return true;
}