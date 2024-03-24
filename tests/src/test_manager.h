#pragma once

#include <defines.h>

#define BYPASS 2  // for the 3 pt true false check, 0 is false, 1 is true, and 2 is bypass

typedef u8 (*PFN_test)();  // define a pointer function for a test method- gives a 3 option true and false, wit true, false, and bypass

// initialize the test manager
void test_manager_init();

// register a test to be run, pass the test in as a pointer function, also pass in a description of the test
void test_manager_register_test(PFN_test, char* desc);

// run all the registered tests
void test_manager_run_tests();