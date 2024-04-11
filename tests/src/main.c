#include "test_manager.h"

#include "memory/linear_allocator_tests.h"
#include "containers/hashtable_tests.h"
#include "containers/freelist_tests.h"
#include "memory/dynamic_allocator_tests.h"

#include <core/logger.h>

int main() {
    // always initialize the test manager first
    test_manager_init();

    // TODO: actually need to add test registrations here
    linear_allocator_register_tests();
    hashtable_register_tests();
    freelist_register_tests();
    dynamic_allocator_register_tests();

    KDEBUG("starting tests...");

    // execute tests
    test_manager_run_tests();

    return 0;
}