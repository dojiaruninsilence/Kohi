#include "test_manager.h"

#include <containers/darray.h>
#include <core/logger.h>
#include <core/kstring.h>
#include <core/clock.h>

// store the data for the tests entry
typedef struct test_entry {
    PFN_test func;  // store pointer function for a test method - this will return the code from the test method
    char* desc;     // store a pointer to the description of the function
} test_entry;

static test_entry* tests;  // define a pointer to a test_entry, tests

// initialize the test manager
void test_manager_init() {
    tests = darray_create(test_entry);  // tests becomes a darray with the size of one test entry
}

// register a test to be run, pass the test in as a pointer function, also pass in a description of the test
void test_manager_register_test(u8 (*PFN_test)(), char* desc) {
    test_entry e;           // define a new resulting test entry, e
    e.func = PFN_test;      // pass the pointer to func
    e.desc = desc;          // pass through the description
    darray_push(tests, e);  // push the new test entry into the tests array
}

// run all the registered tests
void test_manager_run_tests() {
    u32 passed = 0;   // define the number of passes, fails, and skips
    u32 failed = 0;   // define the number of passes, fails, and skips
    u32 skipped = 0;  // define the number of passes, fails, and skips

    u32 count = darray_length(tests);  // get the number of test entries, and store in count

    clock total_time;          // define a clock, total time
    clock_start(&total_time);  // use our function to start the clock, pass in the address to total time

    for (u32 i = 0; i < count; ++i) {
        clock test_time;              // create a new clock, call it test time
        clock_start(&test_time);      // start the new test clock
        u8 result = tests[i].func();  // run the method at index i in tests, and store the results in result
        clock_update(&test_time);     // update the test time clock

        if (result == true) {                       // if results are true
            ++passed;                               // increment the passed count
        } else if (result == BYPASS) {              // if result was bypass
            KWARN("[SKIPPED]: %s", tests[i].desc);  // throw warn wiht desc
            ++skipped;                              // increment the skipped count
        } else {                                    // if results are false
            KERROR("[FAILED]: %s", tests[i].desc);  // throw error message
            ++failed;                               // increment failed count
        }
        char status[20];                                                                                                                               // define a char array wit 20 elements
        string_format(status, failed ? "*** %d FAILED ***" : "SUCCESS", failed);                                                                       // format a string with these inputs for a message to out put - changes on whether it passed or failed
        clock_update(&total_time);                                                                                                                     // update the total time clock
        KINFO("Executed %d of %d (skipped %d) %s (%.6f sec / %.6f sec total)", i + 1, count, skipped, status, test_time.elapsed, total_time.elapsed);  // give info level message of the results and the time it took
    }

    clock_stop(&total_time);  // stop total time clock using our function

    KINFO("Results: %d passed, %d failed, %dskipped.", passed, failed, skipped);  // give info level warning with a tally of the results
}