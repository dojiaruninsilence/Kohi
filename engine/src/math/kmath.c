#include "kmath.h"
#include "platform/platform.h"

#include <math.h>
#include <stdlib.h>

static b8 rand_seeded = FALSE;  // whenever you use a rng, you have to seed it with a certain value

// NOTE: these are here in order to prevent having to import the entire <math.h> everywhere
f32 ksin(f32 x) {
    return sinf(x);
}

f32 kcos(f32 x) {
    return cosf(x);
}

f32 ktan(f32 x) {
    return tanf(x);
}

f32 kacos(f32 x) {
    return acosf(x);
}

f32 ksqrt(f32 x) {
    return sqrtf(x);
}

f32 kabs(f32 x) {
    return fabsf(x);
}

// rng stuff, i really want to beef this area up first thing
i32 krandom() {
    if (!rand_seeded) {                            // if rand seeded is false
        srand((u32)platform_get_absolute_time());  // pass the absolute time into the random gen for a seed
        rand_seeded = TRUE;                        // set seeded to true, so a new one doesnt get passed in
    }
    return rand();  // return a semi randomly gernerated number
}

// random  number within a range
i32 krandom_in_range(i32 min, i32 max) {
    if (!rand_seeded) {                            // if rand seeded is false
        srand((u32)platform_get_absolute_time());  // pass the absolute time into the random gen for a seed
        rand_seeded = TRUE;                        // set seeded to true, so a new one doesnt get passed in
    }
    return (rand() % (max - min + 1)) + min;  // takes the random and performs a modulous(need to look this one uop for sure) on the max minus the min plus 1 and then add min to the reulting number
}

// random float generator
f32 fkrandom() {
    return (float)krandom() / (f32)RAND_MAX;  // call the random func converted to a float, and divide by rand max
}

// random float within a range
f32 fkrandom_in_range(f32 min, f32 max) {
    return min + ((float)krandom() / ((f32)RAND_MAX / (max - min)));
}