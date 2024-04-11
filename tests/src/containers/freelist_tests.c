#include "hashtable_tests.h"
#include "../test_manager.h"
#include "../expect.h"

#include <defines.h>
#include <containers/freelist.h>
#include <core/kmemory.h>

u8 freelist_should_create_and_destroy() {
    // NOTE: creating a small size list, which will trigger a warning
    KDEBUG("The following warning message is intentional.");

    freelist list;

    // get the memory requirement
    u64 memory_requirement = 0;
    u64 total_size = 40;
    freelist_create(total_size, &memory_requirement, 0, 0);

    // allocate and create the freelist
    void* block = kallocate(memory_requirement, MEMORY_TAG_APPLICATION);
    freelist_create(total_size, &memory_requirement, block, &list);

    // verify that the memory was assigned
    expect_should_not_be(0, list.memory);
    // verify that the entire block is free
    u64 free_space = freelist_free_space(&list);
    expect_should_be(total_size, free_space);

    // destroy and verify that the memory was unassigned
    freelist_destroy(&list);
    expect_should_be(0, list.memory);
    kfree(block, memory_requirement, MEMORY_TAG_APPLICATION);

    return true;
}

u8 freelist_should_allocate_one_and_free_one() {
    freelist list;

    // get the memory requirement
    u64 memory_requirement = 0;
    u64 total_size = 512;
    freelist_create(total_size, &memory_requirement, 0, 0);

    // allocate and create the freelist
    void* block = kallocate(memory_requirement, MEMORY_TAG_APPLICATION);
    freelist_create(total_size, &memory_requirement, block, &list);

    // allocate some space
    u32 offset = INVALID_ID;  // start with invalid id, which is a good default since it should never happen
    b8 result = freelist_allocate_block(&list, 64, &offset);
    // verify that result is true, offset should be set to 0
    expect_to_be_true(result);
    expect_should_be(0, offset);

    // verify that the correct amount of space is free
    u64 free_space = freelist_free_space(&list);
    expect_should_be(total_size - 64, free_space);

    // now free the block
    result = freelist_free_block(&list, 64, offset);
    // verify that result is true
    expect_to_be_true(result);

    // verify the entire block is free
    free_space = freelist_free_space(&list);
    expect_should_be(total_size, free_space);

    // destroy and verify that the memory was unassigned
    freelist_destroy(&list);
    expect_should_be(0, list.memory);
    kfree(block, memory_requirement, MEMORY_TAG_APPLICATION);

    return true;
}

u8 freelist_should_allocate_one_and_free_multi() {
    freelist list;

    // get the memory requirement
    u64 memory_requirement = 0;
    u64 total_size = 512;
    freelist_create(total_size, &memory_requirement, 0, 0);

    // allocate and create the freelist
    void* block = kallocate(memory_requirement, MEMORY_TAG_APPLICATION);
    freelist_create(total_size, &memory_requirement, block, &list);

    // allocate some space
    u32 offset = INVALID_ID;  // start with an invalid id, which is a good default since it should never happen
    b8 result = freelist_allocate_block(&list, 64, &offset);
    // verify taht the result is true, offset should be set to 0
    expect_to_be_true(result);
    expect_should_be(0, offset);

    // allocate some more space
    u32 offset2 = INVALID_ID;  // start with an invalid id, which is a good default since it should never happen
    result = freelist_allocate_block(&list, 64, &offset2);
    // verify that result is true, offset should be set to the size of the previous allocation
    expect_to_be_true(result);
    expect_should_be(64, offset2);

    // allocate some more space
    u32 offset3 = INVALID_ID;  // start with an invalid id, which is a good default since it should never happen
    result = freelist_allocate_block(&list, 64, &offset3);
    // verify that result is true, offset should be set to the size of the previous allocation
    expect_to_be_true(result);
    expect_should_be(128, offset3);

    // verify that the correct amount of space is free
    u64 free_space = freelist_free_space(&list);
    expect_should_be(total_size - 192, free_space);

    // now free the middle block
    result = freelist_free_block(&list, 64, offset2);
    // verify that result is true
    expect_to_be_true(result);

    // verify the correct ammount is free
    free_space = freelist_free_space(&list);
    expect_should_be(total_size - 128, free_space);

    // allocate some more space - this should fill the middle block back in
    u32 offset4 = INVALID_ID;  // start with an invalid id, which is a good default since it should never happen
    result = freelist_allocate_block(&list, 64, &offset4);
    // verify that result is true, offset should be set to the size of the previous allocation
    expect_to_be_true(result);
    expect_should_be(offset2, offset4);  // offset should be the same as 2 since it occupies the same space

    // verify that the correct amount of space is free
    free_space = freelist_free_space(&list);
    expect_should_be(total_size - 192, free_space);

    // free the first block and verify space
    result = freelist_free_block(&list, 64, offset);
    expect_to_be_true(result);
    free_space = freelist_free_space(&list);
    expect_should_be(total_size - 128, free_space);

    // free the last block and verify space
    result = freelist_free_block(&list, 64, offset3);
    expect_to_be_true(result);
    free_space = freelist_free_space(&list);
    expect_should_be(total_size - 64, free_space);

    // free the middle block and verify space
    result = freelist_free_block(&list, 64, offset4);
    expect_to_be_true(result);
    free_space = freelist_free_space(&list);
    expect_should_be(total_size, free_space);

    // destroy and verify that the memory was unassigned
    freelist_destroy(&list);
    expect_should_be(0, list.memory);
    kfree(block, memory_requirement, MEMORY_TAG_APPLICATION);

    return true;
}

u8 freelist_should_allocate_one_and_free_multi_varying_sizes() {
    freelist list;

    // get the memory requirement
    u64 memory_requirement = 0;
    u64 total_size = 512;
    freelist_create(total_size, &memory_requirement, 0, 0);

    // allocate and create the freelist
    void* block = kallocate(memory_requirement, MEMORY_TAG_APPLICATION);
    freelist_create(total_size, &memory_requirement, block, &list);

    // allocate some space
    u32 offset = INVALID_ID;  // start with an invalid id, which is a good default since it should never happen
    b8 result = freelist_allocate_block(&list, 64, &offset);
    // verify taht the result is true, offset should be set to 0
    expect_to_be_true(result);
    expect_should_be(0, offset);

    // allocate some more space
    u32 offset2 = INVALID_ID;  // start with an invalid id, which is a good default since it should never happen
    result = freelist_allocate_block(&list, 32, &offset2);
    // verify that result is true, offset should be set to the size of the previous allocation
    expect_to_be_true(result);
    expect_should_be(64, offset2);

    // allocate some more space
    u32 offset3 = INVALID_ID;  // start with an invalid id, which is a good default since it should never happen
    result = freelist_allocate_block(&list, 64, &offset3);
    // verify that result is true, offset should be set to the size of the previous allocation
    expect_to_be_true(result);
    expect_should_be(96, offset3);

    // verify that the correct amount of space is free
    u64 free_space = freelist_free_space(&list);
    expect_should_be(total_size - 160, free_space);

    // now free the middle block
    result = freelist_free_block(&list, 32, offset2);
    // verify that result is true
    expect_to_be_true(result);

    // verify the correct ammount is free
    free_space = freelist_free_space(&list);
    expect_should_be(total_size - 128, free_space);

    // allocate some more space, this time larger than the old middle block. this should have a new offset at the end of the list
    u32 offset4 = INVALID_ID;  // start with an invalid id, which is a good default since it should never happen
    result = freelist_allocate_block(&list, 64, &offset4);
    // verify that result is true, offset should be set to the size of the previous allocation
    expect_to_be_true(result);
    expect_should_be(160, offset4);

    // verify that the correct amount of space is free
    free_space = freelist_free_space(&list);
    expect_should_be(total_size - 192, free_space);

    // free the first block and verify space
    result = freelist_free_block(&list, 64, offset);
    expect_to_be_true(result);
    free_space = freelist_free_space(&list);
    expect_should_be(total_size - 128, free_space);

    // free the last block and verify space
    result = freelist_free_block(&list, 64, offset3);
    expect_to_be_true(result);
    free_space = freelist_free_space(&list);
    expect_should_be(total_size - 64, free_space);

    // free the middle(now end) block and verify space
    result = freelist_free_block(&list, 64, offset4);
    expect_to_be_true(result);
    free_space = freelist_free_space(&list);
    expect_should_be(total_size, free_space);

    // destroy and verify that the memory was unassigned
    freelist_destroy(&list);
    expect_should_be(0, list.memory);
    kfree(block, memory_requirement, MEMORY_TAG_APPLICATION);

    return true;
}

u8 freelist_should_allocate_to_full_and_fail_to_allocate_more() {
    freelist list;

    // get the memory requirement
    u64 memory_requirement = 0;
    u64 total_size = 512;
    freelist_create(total_size, &memory_requirement, 0, 0);

    // allocate and create the freelist
    void* block = kallocate(memory_requirement, MEMORY_TAG_APPLICATION);
    freelist_create(total_size, &memory_requirement, block, &list);

    // allocate all the space
    u32 offset = INVALID_ID;  // start with invalid id, which is a good default since it should never happen
    b8 result = freelist_allocate_block(&list, 512, &offset);
    // verify that result is true, offset should be set to 0
    expect_to_be_true(result);
    expect_should_be(0, offset);

    // verify that the correct amount of space is free
    u64 free_space = freelist_free_space(&list);
    expect_should_be(0, free_space);

    // now try allocating some more
    u32 offset2 = INVALID_ID;
    KDEBUG("The following warning message is intentional.");
    result = freelist_allocate_block(&list, 64, &offset2);
    // verify that result is false
    expect_to_be_false(result);

    // verify that the correct amount of space is free
    free_space = freelist_free_space(&list);
    expect_should_be(0, free_space);

    // destroy and verify that the memory was unassigned
    freelist_destroy(&list);
    expect_should_be(0, list.memory);
    kfree(block, memory_requirement, MEMORY_TAG_APPLICATION);

    return true;
}

void freelist_register_tests() {
    test_manager_register_test(freelist_should_create_and_destroy, "Freelist should create and destroy");
    test_manager_register_test(freelist_should_allocate_one_and_free_one, "Freelist allocate and free one entry.");
    test_manager_register_test(freelist_should_allocate_one_and_free_multi, "Freelist allocate and free multiple entries.");
    test_manager_register_test(freelist_should_allocate_one_and_free_multi_varying_sizes, "Freelist allocate and free multiple entries of varying sizes.");
    test_manager_register_test(freelist_should_allocate_to_full_and_fail_to_allocate_more, "Freelist allocate to full and fail when trying to allocate more.");
}