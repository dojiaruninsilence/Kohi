#pragma once

#include "defines.h"
#include "math_types.h"

// defines for some constants that we will be using
#define K_PI #define K_PI 3.14159265358979323846f      // a floating point representation of pi
#define K_PI_2 2.0f * K_PI                             // a floating point representation of 2 times pi
#define K_HALF_PI 0.5f * K_PI                          // a floating point representation pi times .5
#define K_QUARTER_PI 0.25f * K_PI                      // a floating point representation of pi times .25
#define K_ONE_OVER_PI 1.0f / K_PI                      // a floating point representation 1 divided by pi
#define K_ONE_OVER_TWO_PI 1.0f / K_PI_2                // a floating point representation of 1 divided by (pi times 2)
#define K_SQRT_TWO 1.41421356237309504880f             // a floating point representation the sqaure root of 2
#define K_SQRT_THREE 1.73205080756887729352f           // a floating point representation the square root of 3
#define K_SQRT_ONE_OVER_TWO 0.70710678118654752440f    // a floating point representation the sqaure root of 1 divided by 2
#define K_SQRT_ONE_OVER_THREE 0.57735026918962576450f  // a floating point representation the sqaure root of 1 divided by 3
#define K_DEG2RAD_MULTIPLIER K_PI / 180.0f             // quick way to convert degrees to radians
#define K_RAD2DEG_MULTIPLIER 180.0f / K_PI             // quick way to convert radians to degrees

// some handy defines to have

// the multiplier to convert seconds to milliseconds
#define K_SEC_TO_MS_MULTIPLIER 1000.0f

// the multiplier to convert milliseconds to seconds
#define K_MS_TO_SEC_MULTIPLIER 0.001f

// a huge number that should be larger than any valid number used
#define K_INFINITY 1e30f

// smallest positive number where 1.0 + FLOAT_EPSILON != 0
#define K_FLOAT_EPSILON 1.192092896e-07f  // use for comparrisons of floating point numbers

// general math functions
KAPI f32 ksin(f32 x);   // sin function
KAPI f32 kcos(f32 x);   // cosin function
KAPI f32 ktan(f32 x);   // tangent function
KAPI f32 kacos(f32 x);  // arc cosin function
KAPI f32 ksqrt(f32 x);  // sqare root function
KAPI f32 kabs(f32 x);   // absolute value

// indicates if the value is a power of 2. 0 is considered __not__ a power of 2
// @param value the value to be interpreted
// @returns True if a power of 2, otherwise false
KINLINE b8 is_power_of_2(u64 value) {                     // any time that this function is used, at compile the function call will be removed and this code is just ran
    return (value != 0) && ((value & (value - 1)) == 0);  // uses bit hackery to determine if it is a value of 2 rather than doing divisions and such
}

// basic rng stuffs
// random integers
KAPI i32 krandom();
KAPI i32 krandom_in_range(i32 min, i32 max);

// random floating points
KAPI f32 fkrandom();
KAPI f32 fkrandom_in_range(f32 min, f32 max);

// vector 2 -----------------------------------------------------------------------------------------------------------------------

// @brief creates and returns a new 2-elenent vector using the supplied values

// @param x the x value
// @param y the y value
// @return a 2 element vector

KINLINE vec2 vec2_create(f32 x, f32 y) {
    vec2 out_vector;    // define the array
    out_vector.x = x;   // pass in the x
    out_vector.y = y;   // and the y
    return out_vector;  // return the array
}

// @brief creates and returns a 2 component vector with all components set to 0.0f
KINLINE vec2 vec2_zero() {
    return (vec2){0.0f, 0.0f};
}

// @brief creates and returns a 2 component vector with all components set to 1.0f
KINLINE vec2 vec2_one() {
    return (vec2){1.0f, 1.0f};
}

// @brief creates and returns a 2 component vector pointing up (0, 1)
KINLINE vec2 vec2_up() {
    return (vec2){0.0f, 1.0f};
}

// @brief creates and returns a 2 component vector pointing up (0, -1)
KINLINE vec2 vec2_down() {
    return (vec2){0.0f, -1.0f};
}

// @brief creates and returns a 2 component vector pointing up (-1, 0)
KINLINE vec2 vec2_left() {
    return (vec2){-1.0f, 0.0f};
}

// @brief creates and returns a 2 component vector pointing up (1, 0)
KINLINE vec2 vec2_right() {
    return (vec2){1.0f, 0.0f};
}

// add vector 2s together
// @brief adds vector 1 to vector 0 and returns a copy of the result

// @param vector_0 the first vector
// @param vector_1 the second vector
// @return the resulting vector
KINLINE vec2 vec2_add(vec2 vector_0, vec2 vector_1) {
    return (vec2){                           // return a new vector 2
                  vector_0.x + vector_1.x,   // add both x values
                  vector_0.y + vector_1.y};  // add both y values
}

// subtract vector 2s from each other
// @brief subtracts vector 1 from vector 0 and returns a copy of the result

// @param vector_0 the first vector
// @param vector_1 the second vector
// @return the resulting vector
KINLINE vec2 vec2_sub(vec2 vector_0, vec2 vector_1) {
    return (vec2){                           // return a new vector 2
                  vector_0.x - vector_1.x,   // subtract both x values
                  vector_0.y - vector_1.y};  // subtract both y values
}

// multiply vector 2s together
// @brief multiplies vector 1 by vector 0 and returns a copy of the result

// @param vector_0 the first vector
// @param vector_1 the second vector
// @return the resulting vector
KINLINE vec2 vec2_mul(vec2 vector_0, vec2 vector_1) {
    return (vec2){                           // return a new vector 2
                  vector_0.x * vector_1.x,   // multiply both x values
                  vector_0.y * vector_1.y};  // multiply both y values
}

// divide vector 2s from each other
// @brief divides vector 0 by vector 1 and returns a copy of the result

// @param vector_0 the first vector
// @param vector_1 the second vector
// @return the resulting vector
KINLINE vec2 vec2_div(vec2 vector_0, vec2 vector_1) {
    return (vec2){                           // return a new vector 2
                  vector_0.x / vector_1.x,   // divide both x values
                  vector_0.y / vector_1.y};  // divide both y values
}

// vec2 triangle stuffs---------------------------------------

// @brief returns the squared length of the provided vector

// @param vector the vector to retrieve the squared length of
// @return the squared length
KINLINE f32 vec2_length_squared(vec2 vector) {
    return vector.x * vector.x + vector.y * vector.y;  // return (x squared) plus (y squared) - which tells us how long the vector is squared
}

// @brief returns the length of the provided vector

// @param vector the vector to retrieve the squared length of
// @return the length
KINLINE f32 vec2_length(vec2 vector) {
    return ksqrt(vec2_length_squared(vector));  // return the square root of (x squared) plus (y squared) - which tells us how long the vector is
}

// vec2 normalize stuffs--------------------------------------

// @brief normalizes the provided vector in place to a unit vector
// @param vector a pointer to the vector to be normalized
KINLINE void vec2_normalize(vec2* vector) {
    const f32 length = vec2_length(*vector);  // get the length of the vector
    vector->x /= length;                      // x = x divided by the length
    vector->y /= length;                      // y = y divided by the length
}

// @brief returns a normalized copy of the supplied vector
// @param vector a pointer to the vector to be normalized
// @return a normalized copy of the supplied vector
KINLINE vec2 vec2_normalized(vec2 vector) {
    vec2_normalize(&vector);
    return vector;
}

// check for quality in vectors
// @brief compares all elements of vector_0 and vector_1 and ensures the difference is less than the tolerance
// @param vector_0 the first vector
// @param vector_1 the second vector
// @param tolerance the difference tolerance. typically K_FLOAT_EPSILON or similar
// @return true if within tolerance; otherwise false
KINLINE b8 vec2_compare(vec2 vector_0, vec2 vector_1, f32 tolerance) {
    if (kabs(vector_0.x - vector_1.x) > tolerance) {  // if the absolute value of vec0 x - vec1 x is greater than the tolerance
        return FALSE;                                 // is false
    }

    if (kabs(vector_0.y - vector_1.y) > tolerance) {  // if the absolute value of vec0 y - vec1 y is greater than the tolerance
        return FALSE;                                 // is false
    }
    // if neither triggered is true
    return TRUE;
}

// @brief returns the distance between vector 0 and vector 1
// @param vector_0 the first vector
// @param vector_1 the second vector
// @return the distance btween vector 0 and vector1
KINLINE f32 vec2_distance(vec2 vector_0, vec2 vector_1) {
    vec2 d = (vec2){                           // define a new vec2 d
                    vector_0.x - vector_1.x,   // x0 - x1
                    vector_0.y - vector_1.y};  // y0 - y1
    return vec2_length(d);                     // then use our function to get the length of d
}

// vector 3 --------------------------------------------------------------------------------------------------------------------------------------------------------------

// @brief creates and returns a new 3 element vector using the supplied values

// @param x the x value
// @param y the y value
// @param z the z value
// @return a new 3 element vector
KINLINE vec3 vec3_create(f32 x, f32 y, f32 z) {
    return (vec3){x, y, z};
}

// @brief returns a new vec3 containing the x, y and z components of the supplied vec4, essentially dropping the w component
// @param vector the 4 component vector to extract from
// @return a new vec3
KINLINE vec3 vec3_from_vec4(vec4 vector) {
    return (vec3){vector.x, vector.y, vector.z};
}

// @brief returns a new vec4 containing the x, y and z components of the supplied vec3 and w for w
// @param vector the 3 component vector
// @param w the w component
// @return a new vec4
KINLINE vec4 vec3_to_vec4(vec3 vector, f32 w) {
    return (vec4){vector.x, vector.y, vector.z, w};
}

// @brief creates and returns a 3 component vector with all components set to 0.0f
KINLINE vec3 vec3_zero() {
    return (vec3){0.0f, 0.0f, 0.0f};
}

// @brief creates and returns a 3 component vector with all components set to 1.0f
KINLINE vec3 vec3_one() {
    return (vec3){1.0f, 1.0f, 1.0f};
}

// @brief creates and returns a 3 component vector pointing up (0, 1, 0)
KINLINE vec3 vec3_up() {
    return (vec3){0.0f, 1.0f, 0.0f};
}

// @brief creates and returns a 3 component vector pointing up (0, -1, 0)
KINLINE vec3 vec3_down() {
    return (vec3){0.0f, -1.0f, 0.0f};
}

// @brief creates and returns a 3 component vector pointing up (-1, 0, 0)
KINLINE vec3 vec3_left() {
    return (vec3){-1.0f, 0.0f, 0.0f};
}

// @brief creates and returns a 3 component vector pointing up (1, 0, 0)
KINLINE vec3 vec3_right() {
    return (vec3){1.0f, 0.0f, 0.0f};
}

// @brief creates and returns a 3 component vector pointing foreward (0, 0, -1)
KINLINE vec3 vec3_left() {
    return (vec3){0.0f, 0.0f, -1.0f};
}

// @brief creates and returns a 3 component vector pointing foreward (0, 0, 1)
KINLINE vec3 vec3_right() {
    return (vec3){0.0f, 0.0f, 1.0f};
}

// add vector 3s together
// @brief adds vector 1 to vector 0 and returns a copy of the result

// @param vector_0 the first vector
// @param vector_1 the second vector
// @return the resulting vector
KINLINE vec3 vec3_add(vec3 vector_0, vec3 vector_1) {
    return (vec3){                           // return a new vector 3
                  vector_0.x + vector_1.x,   // add both x values
                  vector_0.y + vector_1.y,   // add both y values
                  vector_0.z + vector_1.z};  // add both z values
}

// subract vector 3s from each other
// @brief subtracts vector 1 from vector 0 and returns a copy of the result

// @param vector_0 the first vector
// @param vector_1 the second vector
// @return the resulting vector
KINLINE vec3 vec3_sub(vec3 vector_0, vec3 vector_1) {
    return (vec3){                           // return a new vector 3
                  vector_0.x - vector_1.x,   // subtract both x values
                  vector_0.y - vector_1.y,   // subtract both y values
                  vector_0.z - vector_1.z};  // subtract both z values
}

// multiply vector 3s together
// @brief multiplies vector 1 by vector 0 and returns a copy of the result

// @param vector_0 the first vector
// @param vector_1 the second vector
// @return the resulting vector
KINLINE vec3 vec3_mul(vec3 vector_0, vec3 vector_1) {
    return (vec3){                           // return a new vector 3
                  vector_0.x * vector_1.x,   // multiply both x values
                  vector_0.y * vector_1.y,   // multiply both y values
                  vector_0.z * vector_1.z};  // multiply both z values
}

// @brief multiplies all elements of vector_0 by scalar and returns a copy of the result
// @param vector_0 the vector to be multiplied
// @param scalar the scalar value
// @return a copy of the resulting vector
KINLINE vec3 vec3_mul_scalar(vec3 vector_0, f32 scalar) {
    return (vec3){                       // return a new vector 3
                  vector_0.x * scalar,   // multiply x by the scalar
                  vector_0.y * scalar,   // multiply y by the scalar
                  vector_0.z * scalar};  // multiply z by the scalar
}

// divide vector 3s from each other
// @brief divide vector 0 by vector 1 and returns a copy of the result

// @param vector_0 the first vector
// @param vector_1 the second vector
// @return the resulting vector
KINLINE vec3 vec3_div(vec3 vector_0, vec3 vector_1) {
    return (vec3){                           // return a new vector 3
                  vector_0.x / vector_1.x,   // divide both x values
                  vector_0.y / vector_1.y,   // divide both y values
                  vector_0.z / vector_1.z};  // divide both z values
}

// @brief returns the squared length of the provided vector

// @param vector the vector to retrieve the squared length of
// @return the squared length
KINLINE f32 vec3_length_squared(vec3 vector) {
    return vector.x * vector.x + vector.y * vector.y + vector.z * vector.z;  // return (x squared) plus (y squared) plus (z squared) - which tells us how long the vector is squared
}

// @brief returns the length of the provided vector

// @param vector the vector to retrieve the squared length of
// @return the length
KINLINE f32 vec3_length(vec3 vector) {
    return ksqrt(vec3_length_squared(vector));  // return the square root of (x squared) plus (y squared) - which tells us how long the vector is
}

// @brief normalizes the provided vector in place to a unit vector
// @param vector a pointer to the vector to be normalized
KINLINE void vec3_normalize(vec3* vector) {
    const f32 length = vec3_length(*vector);  // get the length of the vector
    vector->x /= length;                      // x = x divided by the length
    vector->y /= length;                      // y = y divided by the length
    vector->z /= length;                      // z = z divided by the length
}

// @brief returns a normalized copy of the supplied vector
// @param vector a pointer to the vector to be normalized
// @return a normalized copy of the supplied vector
KINLINE vec3 vec3_normalized(vec3 vector) {
    vec3_normalize(&vector);
    return vector;
}

// @brief returns the dot product between the provided vectors. typically used to calculate the difference in direction
// @param vector_0 the first vector
// @param vector_1 the second vector
// @return the dot product
KINLINE f32 vec3_dot(vec3 vector_0, vec3 vector_1) {
    f32 p = 0;                     // define a float p
    p += vector_0.x * vector_1.x;  // p = p + (x0 * x1)
    p += vector_0.y * vector_1.y;  // p = p + (y0 * y1)
    p += vector_0.z * vector_1.z;  // p = p + (z0 * z1)
    return p;                      // p = (x0 * x1) + (y0 * y1) + (z0 * z1)
}

// @brief calculates and returns the cross product of the supplied vectors. the cross product is a new vector which is orthoganal to both provided vectors
// @param vector_0 the first vector
// @param vector_1 the second vector
// @return the cross product
KINLINE vec3 vec3_cross(vec3 vector_0, vec3 vector_1) {
    return (vec3){
        vector_0.y * vector_1.z - vector_0.z * vector_1.y,   // (y0 * z1) - (z0 * y1)
        vector_0.z * vector_1.x - vector_0.x * vector_1.z,   // (z0 * x1) - (x0 * z1)
        vector_0.x * vector_1.y - vector_0.y * vector_1.x};  // (x0 * y1) - (y0 * x1)
}

// check for quality in vectors
// @brief compares all elements of vector_0 and vector_1 and ensures the difference is less than the tolerance
// @param vector_0 the first vector
// @param vector_1 the second vector
// @param tolerance the difference tolerance. typically K_FLOAT_EPSILON or similar
// @return true if within tolerance; otherwise false
KINLINE b8 vec3_compare(vec3 vector_0, vec3 vector_1, f32 tolerance) {
    if (kabs(vector_0.x - vector_1.x) > tolerance) {  // if the absolute value of vec0 x - vec1 x is greater than the tolerance
        return FALSE;                                 // is false
    }

    if (kabs(vector_0.y - vector_1.y) > tolerance) {  // if the absolute value of vec0 y - vec1 y is greater than the tolerance
        return FALSE;                                 // is false
    }

    if (kabs(vector_0.z - vector_1.z) > tolerance) {  // if the absolute value of vec0 z - vec1 z is greater than the tolerance
        return FALSE;                                 // is false
    }
    // if none triggered is true
    return TRUE;
}

// @brief returns the distance between vector 0 and vector 1
// @param vector_0 the first vector
// @param vector_1 the second vector
// @return the distance btween vector 0 and vector1
KINLINE f32 vec3_distance(vec3 vector_0, vec3 vector_1) {
    vec3 d = (vec3){                           // define a new vec2 d
                    vector_0.x - vector_1.x,   // x0 - x1
                    vector_0.y - vector_1.y,   // y0 - y1
                    vector_0.z - vector_1.z};  // z0 - z1
    return vec3_length(d);                     // then use our function to get the length of d
}

// vector 4 --------------------------------------------------------------------------------------------------------------------------------------------------------------

// @brief creates and returns a new 4 element vector using the supplied values

// @param x the x value
// @param y the y value
// @param z the z value
// @param w the w value
// @return a new 4 element vector
KINLINE vec4 vec4_create(f32 x, f32 y, f32 z, f32 w) {
    vec4 out_vector;
#if defined(KUSE_SIMD)
    out_vector.data = _mm_setr_ps(x, y, z, w);
#else
    out_vector.x = x;
    out_vector.y = y;
    out_vector.z = z;
    out_vector.w = w;
#endif
    return out_vector;
}

// @brief returns a new vec3 containing the x, y and z components of the supplied vec4, essentially dropping the w component
// @param vector the 4 component vector to extract from
// @return a new vec3
KINLINE vec3 vec4_to_vec3(vec4 vector) {
    return (vec3){vector.x, vector.y, vector.z};
}

// @brief returns a new vec4 using vector as the x, y and z components and w for w
// @param vector the vec3 - three component vector
// @param w the w component
// @return a new vec4
KINLINE vec4 vec4_from_vec3(vec3 vector, f32 w) {
#if defined(KUSE_SIMD)
    vec4 out_vector;
    out_vector.data = _mm_setr_ps(x, y, z, w);
    return out_vector;
#else
    return (vec4){vector.x, vector.y, vector.z, w};
#endif
}

// @brief creates and returns a 4 component vector with all components set to 0.0f
KINLINE vec4 vec4_zero() {
    return (vec4){0.0f, 0.0f, 0.0f, 0.0f};
}

// @brief creates and returns a 4 component vector with all components set to 1.0f
KINLINE vec4 vec4_one() {
    return (vec4){1.0f, 1.0f, 1.0f, 1.0f};
}

// add vector 4s together
// @brief adds vector 1 to vector 0 and returns a copy of the result

// @param vector_0 the first vector
// @param vector_1 the second vector
// @return the resulting vector
KINLINE vec4 vec4_add(vec4 vector_0, vec4 vector_1) {
    vec4 result;                                                           // define vec 4 result
    for (u64 i = 0; i < 4; ++i) {                                          // iterate through the components
        result.elements[i] = vector_0.elements[i] + vector_1.elements[i];  // result at index i = i0 + i1
    }
    return result;
}

// subtract vector 4s from each other
// @brief subtracts vector 1 from vector 0 and returns a copy of the result

// @param vector_0 the first vector
// @param vector_1 the second vector
// @return the resulting vector
KINLINE vec4 vec4_sub(vec4 vector_0, vec4 vector_1) {
    vec4 result;                                                           // define vec 4 result
    for (u64 i = 0; i < 4; ++i) {                                          // iterate through the components
        result.elements[i] = vector_0.elements[i] - vector_1.elements[i];  // result at index i = i0 - i1
    }
    return result;
}

// multiply vector 4s together
// @brief multiplies vector 1 by vector 0 and returns a copy of the result

// @param vector_0 the first vector
// @param vector_1 the second vector
// @return the resulting vector
KINLINE vec4 vec4_mul(vec4 vector_0, vec4 vector_1) {
    vec4 result;                                                           // define vec 4 result
    for (u64 i = 0; i < 4; ++i) {                                          // iterate through the components
        result.elements[i] = vector_0.elements[i] * vector_1.elements[i];  // result at index i = i0 + i1
    }
    return result;
}

// divide vector 4s from each other
// @brief divides vector 0 by vector 1 and returns a copy of the result

// @param vector_0 the first vector
// @param vector_1 the second vector
// @return the resulting vector
KINLINE vec4 vec4_div(vec4 vector_0, vec4 vector_1) {
    vec4 result;                                                           // define vec 4 result
    for (u64 i = 0; i < 4; ++i) {                                          // iterate through the components
        result.elements[i] = vector_0.elements[i] / vector_1.elements[i];  // result at index i = i0 - i1
    }
    return result;
}

// @brief returns the squared length of the provided vector

// @param vector the vector to retrieve the squared length of
// @return the squared length
KINLINE f32 vec4_length_squared(vec4 vector) {
    return vector.x * vector.x + vector.y * vector.y + vector.z * vector.z + vector.w * vector.w;  // return (x squared) plus (y squared) plus (z squared) plus (w squared) - which tells us how long the vector is squared
}

// @brief returns the length of the provided vector

// @param vector the vector to retrieve the squared length of
// @return the length
KINLINE f32 vec4_length(vec4 vector) {
    return ksqrt(vec4_length_squared(vector));  // return the square root of (x squared) plus (y squared) plus (z squared) plus (w squared)  - which tells us how long the vector is
}

// @brief normalizes the provided vector in place to a unit vector
// @param vector a pointer to the vector to be normalized
KINLINE void vec4_normalize(vec4* vector) {
    const f32 length = vec4_length(*vector);  // get the length of the vector
    vector->x /= length;                      // x = x divided by the length
    vector->y /= length;                      // y = y divided by the length
    vector->z /= length;                      // z = z divided by the length
    vector->w /= length;                      // w = w divided by the length
}

// @brief returns a normalized copy of the supplied vector
// @param vector a pointer to the vector to be normalized
// @return a normalized copy of the supplied vector
KINLINE vec4 vec4_normalized(vec4 vector) {
    vec4_normalize(&vector);
    return vector;
}

KINLINE f32 vec4_dot_f32(
    f32 a0, f32 a1, f32 a2, f32 a3,
    f32 b0, f32 b1, f32 b2, f32 b3) {
    f32 p;
    p =
        a0 * b0 +
        a1 * b1 +
        a2 * b2 +
        a3 * b3;
    return p;
}