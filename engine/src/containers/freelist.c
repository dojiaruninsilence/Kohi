#include "freelist.h"

#include "core/kmemory.h"
#include "core/logger.h"

// info that is kept for each node
typedef struct freelist_node {
    u64 offset;                  // how far from the first node
    u64 size;                    // stride
    struct freelist_node* next;  // pointer to the node next in the free list
} freelist_node;

// where the internal state of a freelist is kept
typedef struct internal_state {
    u64 total_size;        // total memory the free list is covering
    u64 max_entries;       // max entries into the freelist
    freelist_node* head;   // pointer to where the head of the list is - the very first node
    freelist_node* nodes;  // an array of nodes in the free list
} internal_state;

// private functions
freelist_node* get_node(freelist* list);
void return_node(freelist* list, freelist_node* node);

// @brief creates a new free list or obtains the memory requirement for one. call twice; once passing 0 to memory to obtain memory requirement,
// and a second time passing an allocated block of memory
// @param total_size the total size in bytes that the free list should track.
// @param memory_requirement a pointer to hold memory requirement for the free list itself
// @param memory 0, or a pre-allocated block of memory for the free list to use
// @param out_list a pointer to hold the created free list
void freelist_create(u64 total_size, u64* memory_requirement, void* memory, freelist* out_list) {
    // enough space to hold the state, plus an array for all the nodes
    u64 max_entries = (total_size / sizeof(void*));  // NOTE: this may have a remainder, but that is ok
    *memory_requirement = sizeof(internal_state) + (sizeof(freelist_node) * max_entries);
    if (!memory) {
        return;
    }

    // if the memory required is too small, should warn about it being wasteful to use
    u64 mem_min = (sizeof(internal_state) + sizeof(freelist_node)) * 8;  // multiplied by for 8 bytes
    if (total_size < mem_min) {
        KWARN("Freelists are very inefficient with amounts of memory less than %iB; it is recommended to not use this function in this case.", mem_min);
    }

    // assign the passed in allocated memory to hold the state and nodes
    out_list->memory = memory;

    // the block's layout is head* first, then array of available nodes
    // pass through all the infos for the internal state, and point the nodes array where it needs to go
    kzero_memory(out_list->memory, *memory_requirement);
    internal_state* state = out_list->memory;
    state->nodes = (void*)(out_list->memory + sizeof(internal_state));
    state->max_entries = max_entries;
    state->total_size = total_size;

    // head is set to the address of the first node in the nodes array
    // pass in the initial values
    state->head = &state->nodes[0];
    state->head->offset = 0;
    state->head->size = total_size;
    state->head->next = 0;

    // invalidate the offset and size for all but the first node. the invalid value will be checked for when seeking a new node from the list
    for (u64 i = 1; i < state->max_entries; ++i) {  // start at one cause zero is the head
        state->nodes[i].offset = INVALID_ID;
        state->nodes[i].size = INVALID_ID;
    }
}

// @brief destroys the provided list
// @param list the list to be destroyed
void freelist_destroy(freelist* list) {
    if (list && list->memory) {
        // just zero out the memory before giving it back
        internal_state* state = list->memory;
        kzero_memory(list->memory, sizeof(internal_state) + sizeof(freelist_node) * state->max_entries);
        list->memory = 0;
    }
}

// @brief attempts to find a free block of memory of the given size.
// @param list a pointer to the list to search.
// @param size the size to allocate
// @param out_offset a pointer to hold the offset to the allocated memory
// @return b8 true if a block of memory was found and allocated; otherwise false
b8 freelist_allocate_block(freelist* list, u64 size, u64* out_offset) {
    if (!list || !out_offset || !list->memory) {
        return false;
    }
    internal_state* state = list->memory;
    // use these to leap frog through the list
    freelist_node* node = state->head;  // current node, start at the head
    freelist_node* previous = 0;        // previous starts null
    while (node) {
        if (node->size == size) {
            // exact match. just return the node
            *out_offset = node->offset;
            freelist_node* node_to_return = 0;  // define the node to be returned
            if (previous) {                     // if the node currently selected isnt the head
                previous->next = node->next;    // the node behind this one now points to the one in front of this one
                node_to_return = node;          // this is the node to use
            } else {
                // this node is the head of the list. reassign the head and return the previous head node
                node_to_return = state->head;  // return the head
                state->head = node->next;      // head is now the next node in the list
            }
            return_node(list, node_to_return);
            return true;
        } else if (node->size > size) {
            // node is larger. deduct the memory from it and move the offset by that ammount
            *out_offset = node->offset;
            node->size -= size;
            node->offset += size;
            return true;
        }

        // the leap frog thing
        previous = node;
        node = node->next;
    }

    u64 free_space = freelist_free_space(list);
    KWARN("freelist_find_block, no block with enough free space found (requested: %lluB, available: %lluB).", size, free_space);
    return false;
}

// @brief attempts to free a block of memory at the given offset, and of the given size. can faile if invalid data is passed.
// @param list a pointer to the list to be free from
// @param size the size to be freed
// @param offset the offset to free at
// @return b8 true if successful; otherwise false. false should be treated as an error
b8 freelist_free_block(freelist* list, u64 size, u64 offset) {
    if (!list || !list->memory || !size) {
        return false;
    }
    internal_state* state = list->memory;
    freelist_node* node = state->head;
    freelist_node* previous = 0;
    if (!node) {
        // check for the case where the entire thing is allocated in one block, in this case a new node is needed at the head
        freelist_node* new_node = get_node(list);
        new_node->offset = offset;
        new_node->size = size;
        new_node->next = 0;
        state->head = new_node;
        return true;
    } else {
        while (node) {
            if (node->offset == offset) {
                // can just be appended to this node
                node->size += size;

                // check if this then connects the range between this and the next node, and if so, combine them and return the second node..
                if (node->next && node->next->offset == node->offset + node->size) {
                    node->size += node->next->size;
                    freelist_node* next = node->next;
                    node->next = node->next->next;
                    return_node(list, next);
                }
                return true;
            } else if (node->offset > offset) {
                // iterated beyond the space to be freed. need a new node
                freelist_node* new_node = get_node(list);
                new_node->offset = offset;
                new_node->size = size;

                // if there is a previous node, the new node should be inserted between this and it
                if (previous) {
                    previous->next = new_node;
                    new_node->next = node;
                } else {
                    // otherwise, the new mode becomes the head
                    new_node->next = node;
                    state->head = new_node;
                }

                // double check the next node to see if it can be joined
                if (new_node->next && new_node->offset + new_node->size == new_node->next->offset) {
                    new_node->size += new_node->next->size;
                    freelist_node* rubbish = new_node->next;
                    new_node->next = rubbish->next;
                    return_node(list, rubbish);
                }

                // double check the previous node to see if the new node can be joined to it
                if (previous && previous->offset + previous->size == new_node->offset) {
                    previous->size += new_node->size;
                    freelist_node* rubbish = new_node;
                    previous->next = rubbish->next;
                    return_node(list, rubbish);
                }

                return true;
            }

            previous = node;
            node = node->next;
        }
    }

    KWARN("Unable to find block to be freed. Corruption possible?");
    return false;
}

b8 freelist_resize(freelist* list, u64* memory_requirement, void* new_memory, u64 new_size, void** out_old_memory) {
    if (!list || !memory_requirement || ((internal_state*)list->memory)->total_size > new_size) {
        return false;
    }

    // enough space to hold the state, plus an array for all nodes
    u64 max_entries = (new_size / sizeof(void*));  // NOTE: this may have a remainder, but thats ok
    *memory_requirement = sizeof(internal_state) + (sizeof(freelist_node) * max_entries);
    if (!new_memory) {
        return true;
    }

    // assign the old pointer so that it can be freed
    *out_old_memory = list->memory;

    // copy over the old state to the new
    internal_state* old_state = (internal_state*)list->memory;
    u64 size_diff = new_size - old_state->total_size;

    // setup the new memory
    list->memory = new_memory;

    // the block's layout is head first, then the array of available nodes
    kzero_memory(list->memory, *memory_requirement);

    // setup the new state
    internal_state* state = (internal_state*)list->memory;
    state->nodes = (void*)(list->memory + sizeof(internal_state));
    state->max_entries = max_entries;
    state->total_size = new_size;

    // invalidate the offset and size for all but the first node. the invalid value will be checked for when seeking a new node from the list
    for (u64 i = 1; i < state->max_entries; ++i) {
        state->nodes[i].offset = INVALID_ID;
        state->nodes[i].size = INVALID_ID;
    }

    state->head = &state->nodes[0];  // head is the first index in the arra of nodes

    // copy over the nodes
    freelist_node* new_list_node = state->head;
    freelist_node* old_node = old_state->head;
    if (!old_node) {
        // if there is no head, then the entire list is allocated. in this case, the head should be set to the difference
        // of the space now available, and at the end of the list
        state->head->offset = old_state->total_size;
        state->head->size = size_diff;
        state->head->next = 0;
    } else {
        // iterate the old nodes
        while (old_node) {
            // get a new node, copy the offset and size, and set next to it
            freelist_node* new_node = get_node(list);
            new_node->offset = old_node->offset;
            new_node->size = old_node->size;
            new_node->next = 0;
            new_list_node->next = new_node;
            // move to the next entry
            new_list_node = new_list_node->next;

            if (old_node->next) {
                // if there is another node move on.
                old_node = old_node->next;
            } else {
                // reached the end of the list, check if it extends to the end of the block. if so, just append to the size.
                // otherwise create a new node and attach to it
                if (old_node->offset + old_node->size == old_state->total_size) {
                    new_node->size += size_diff;
                } else {
                    freelist_node* new_node_end = get_node(list);
                    new_node_end->offset = old_state->total_size;
                    new_node_end->size = size_diff;
                    new_node_end->next = 0;
                    new_node->next = new_node_end;
                }
                break;
            }
        }
    }

    return true;
}

// @brief clears the free list
// @param list the list to be cleared
void freelist_clear(freelist* list) {
    if (!list || !list->memory) {
        return;
    }

    internal_state* state = list->memory;
    // invalidate the offset and size for all but the first node.  the invalid value will be checked for when seeking a new node from the list
    for (u64 i = 1; i < state->max_entries; ++i) {
        state->nodes[i].offset = INVALID_ID;
        state->nodes[i].size = INVALID_ID;
    }

    // reset the head to occupy the entire thing
    state->head->offset = 0;
    state->head->size = state->total_size;
    state->head->next = 0;
}

// @brief returns the amount of free space in this list. NOTE: since this has to iterate the entire internal list,
// this can be an expensive operation. use sparingly
// @param list a pointer to the list to obtain from
// @return the amount of free space in bytes
u64 freelist_free_space(freelist* list) {
    if (!list || !list->memory) {
        return 0;
    }

    u64 running_total = 0;
    internal_state* state = list->memory;
    freelist_node* node = state->head;
    while (node) {
        running_total += node->size;
        node = node->next;
    }

    return running_total;
}

freelist_node* get_node(freelist* list) {
    internal_state* state = list->memory;
    for (u64 i = 1; i < state->max_entries; ++i) {
        if (state->nodes[i].offset == INVALID_ID) {
            return &state->nodes[i];
        }
    }

    // return nothing if no nodes are available
    return 0;
}

void return_node(freelist* list, freelist_node* node) {
    node->offset = INVALID_ID;
    node->size = INVALID_ID;
    node->next = 0;
}
