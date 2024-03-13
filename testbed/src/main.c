#include <core/logger.h>
#include <core/asserts.h>

// TODO: test
#include <platform/platform.h>

int main(void) {
    KFATAL("a test message: %f", 3.14f);
    KERROR("a test message: %f", 3.14f);
    KWARN("a test message: %f", 3.14f);
    KINFO("a test message: %f", 3.14f);
    KDEBUG("a test message: %f", 3.14f);
    KTRACE("a test message: %f", 3.14f);

    platform_state state;
	if(platform_startup(&state, "Kohi engine testbed", 100, 100, 1280, 720)) {
        while(TRUE) {
            platform_pump_messages(&state);
        }
    }

    platform_shutdown(&state);

    return 0;
}