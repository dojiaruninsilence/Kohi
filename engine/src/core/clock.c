#include "clock.h"

#include "platform/platform.h"

// this will most likely be added too

// update the elapsed time
void clock_update(clock* clock) {
    if (clock->start_time != 0) {                                           // if the clock has actually changed
        clock->elapsed = platform_get_absolute_time() - clock->start_time;  // set clock elapsed to the time the os provides minus the start time
    }
}

// start a clock
void clock_start(clock* clock) {
    clock->start_time = platform_get_absolute_time();  // get the time from the os
    clock->elapsed = 0;                                // reset the elapsed time
}

// stop a clock
void clock_stop(clock* clock) {
    clock->start_time = 0;  // reset the start time
}