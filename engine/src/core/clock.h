#pragma once

#include "defines.h"

typedef struct clock {
    f64 start_time;
    f64 elapsed;
} clock;

// updates the provided clock. should be called just before checking elapsed time.
// has no effect on non-started clocks.
KAPI void clock_update(clock* clock);  // perform calcs to get the elapsed time

// starts the provided clock. resets elapsed time.
KAPI void clock_start(clock* clock);  // begins to track the elapsed time

// stops the provided clock. does not reset the elapsed time.
KAPI void clock_stop(clock* clock);  // stops and prevents further updates