#include "linear_allocator_tests.h"
#include "../test_manager.h"
#include "../expect.h"

#include <defines.h>

#include <memory/linear_allocator.h>

// test the linear allocators ability to create and destroy
u8 linear_allocator_should_create_and_destroy() {
    linear_allocator alloc;                           // create a new linear allocator address, alloc
    linear_allocator_create(sizeof(u64), 0, &alloc);  // and create the linear allocator with the size of a u64, and pass the address alloc to ii

    expect_should_not_be(0, alloc.memory);            // run expect should not be with alloc.memory and 0. if they are the same is a failure - says alloc memory should not be zero
    expect_should_be(sizeof(u64), alloc.total_size);  // check if alloc total size is the same size as a u64. if its not then its a failure
    expect_should_be(0, alloc.allocated);             // check if alloc allocated is 0, if its not then its a failure

    linear_allocator_destroy(&alloc);

    expect_should_be(0, alloc.memory);      // run expect should be with alloc.memory and 0. should be the same, if not a failure
    expect_should_be(0, alloc.total_size);  // check if alloc total size is 0. if its not then its a failure
    expect_should_be(0, alloc.allocated);   // check if alloc allocated is 0, if its not then its a failure

    return true;  // test is success
}

// create a linear allocation with one allocation that fills the entire block
u8 linear_allocator_single_allocation_all_space() {
    linear_allocator alloc;                           // define a new linear allocator, alloc
    linear_allocator_create(sizeof(u64), 0, &alloc);  // create the linear allocation, the size of a single u64, it will allocate its own memory, and the address of alloc

    // single allocation
    void* block = linear_allocator_allocate(&alloc, sizeof(u64));  // allocate memory for the linear allocator at the address of alloc, with the size of a single u64

    // validate it
    expect_should_not_be(0, block);                  // there should be something in the block, if not it should fail
    expect_should_be(sizeof(u64), alloc.allocated);  // if the memory isnt a u64 then it fails

    linear_allocator_destroy(&alloc);  // destroy the allocator

    return true;  // test is success
}

// create a linear allocator with the max size and fill it full of allocations
u8 linear_allocator_multi_allocation_all_space() {
    u64 max_allocs = 1024;                                         // define max allocs to 1024
    linear_allocator alloc;                                        // define a linear allocator
    linear_allocator_create(sizeof(u64) * max_allocs, 0, &alloc);  // create the linear allocator with the size of a u64 times max allocs, give it the address in alloc

    // multiple allocations - full
    void* block;                                                 // define a pointer to a block of memory
    for (u64 i = 0; i < max_allocs; ++i) {                       // iterate through all of the max allocs
        block = linear_allocator_allocate(&alloc, sizeof(u64));  // allocate a u64 to the block of memory, move the pointer and all that
        // validate it
        expect_should_not_be(0, block);                            // the block should be full, so it should not have nothing in it, if its empty it failed
        expect_should_be(sizeof(u64) * (i + 1), alloc.allocated);  // the block should be full, if it isnt then it failed
    }
    linear_allocator_destroy(&alloc);  // destroy the allocation, pass in the address of alloc

    return true;  // test is success
}

// test to overfill a linear allocator
u8 linear_allocator_multi_allocation_over_allocate() {
    u64 max_allocs = 3;                                            // define max allocs to 3
    linear_allocator alloc;                                        // define a linear allocator, alloc
    linear_allocator_create(sizeof(u64) * max_allocs, 0, &alloc);  // create the allocator, with the size of a u64 times max allocs, it will allocate its own mwmory, and give it the address in alloc

    // multiple allocations - full
    void* block;                                                 // define a pointer to a block of memory
    for (u64 i = 0; i < max_allocs; ++i) {                       // iterate through the max allocs
        block = linear_allocator_allocate(&alloc, sizeof(u64));  // allocate a u64 to the linear allocator
        // validate it
        expect_should_not_be(0, block);                            // the block should have something in it, fails if it is 0
        expect_should_be(sizeof(u64) * (i + 1), alloc.allocated);  // it should have 3 u64s in it
    }

    KDEBUG("note: the following error is intentionally caused by this test.");

    // ask for one more allocation. should error and return and return 0
    block = linear_allocator_allocate(&alloc, sizeof(u64));
    // validate it - allocated should be unchanged
    expect_should_be(0, block);                                     // allocation should be zero
    expect_should_be(sizeof(u64) * (max_allocs), alloc.allocated);  // and this should not have changed

    linear_allocator_destroy(&alloc);  // destroy the allocatot, pass in the address in alloc

    return true;  // test is success
}

// fill a linear allocator and then free everything in the allocator, before destroying, and verify that everything had been freed and the pointer reset
u8 linear_allocator_multi_allocation_all_space_then_free() {
    u64 max_allocs = 1024;                                         // define max allocs to 1024
    linear_allocator alloc;                                        // define a linear allocator
    linear_allocator_create(sizeof(u64) * max_allocs, 0, &alloc);  // create the linear allocator with the size of a u64 times max allocs, give it the address in alloc

    // multiple allocations - full
    void* block;                                                 // define a pointer to a block of memory
    for (u64 i = 0; i < max_allocs; ++i) {                       // iterate through all of the max allocs
        block = linear_allocator_allocate(&alloc, sizeof(u64));  // allocate a u64 to the block of memory, move the pointer and all that
        // validate it
        expect_should_not_be(0, block);                            // the block should be full, so it should not have nothing in it, if its empty it failed
        expect_should_be(sizeof(u64) * (i + 1), alloc.allocated);  // the block should be full, if it isnt then it failed
    }

    // validate that the pointer is reset
    linear_allocator_free_all(&alloc);     // free all the allocations in the linear allocator
    expect_should_be(0, alloc.allocated);  // the memory should now be zero, if not it fails

    linear_allocator_destroy(&alloc);  // destroy the allocation, pass in the address to alloc

    return true;  // test is success
}

void linear_allocator_register_tests() {
    test_manager_register_test(linear_allocator_should_create_and_destroy, "linear allocator should create and destroy");
    test_manager_register_test(linear_allocator_single_allocation_all_space, "linear allocator should create and destroy");
    test_manager_register_test(linear_allocator_multi_allocation_all_space, "linear allocator should create and destroy");
    test_manager_register_test(linear_allocator_multi_allocation_over_allocate, "linear allocator should create and destroy");
    test_manager_register_test(linear_allocator_multi_allocation_all_space_then_free, "linear allocator should create and destroy");
}