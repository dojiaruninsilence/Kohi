#pragma once

#include "defines.h"
#include "math_types.h"

#include "core/kmemory.h"

// defines for some constants that we will be using
#define K_PI 3.14159265358979323846f                   // a floating point representation of pi
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
        return false;                                 // is false
    }

    if (kabs(vector_0.y - vector_1.y) > tolerance) {  // if the absolute value of vec0 y - vec1 y is greater than the tolerance
        return false;                                 // is false
    }
    // if neither triggered is true
    return true;
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
KINLINE vec3 vec3_forward() {
    return (vec3){0.0f, 0.0f, -1.0f};
}

// @brief creates and returns a 3 component vector pointing foreward (0, 0, 1)
KINLINE vec3 vec3_back() {
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
        return false;                                 // is false
    }

    if (kabs(vector_0.y - vector_1.y) > tolerance) {  // if the absolute value of vec0 y - vec1 y is greater than the tolerance
        return false;                                 // is false
    }

    if (kabs(vector_0.z - vector_1.z) > tolerance) {  // if the absolute value of vec0 z - vec1 z is greater than the tolerance
        return false;                                 // is false
    }
    // if none triggered is true
    return true;
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

// the dot product of two vec4s - this is super important for projection stuffs need to learn more
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

// 4 by 4 matrices ---------------------------------------------------------------------------------------------------------------------------------------------------------------

// creates and returns an identity matrix
// {
//  {1, 0, 0, 0},
//  {0, 1, 0, 0},
//  {0, 0, 1, 0},
//  {0, 0, 0, 1},
// }
// @ return a new identity matrix - the default matrix - shown above - represents a neutral scale
KINLINE mat4 mat4_identity() {
    mat4 out_matrix;                                  // define a mat 4 call it out matrix
    kzero_memory(out_matrix.data, sizeof(f32) * 16);  // call our function to zero out the memory a the location of the matrix data, use the size of a 32 bit floating point times 16 for the size
    out_matrix.data[0] = 1.0f;                        // these values create the default shown above
    out_matrix.data[5] = 1.0f;
    out_matrix.data[10] = 1.0f;
    out_matrix.data[15] = 1.0f;
    return out_matrix;
}

// multiply the collums of one by the rows of the other
// @brief returns the result of multiplyling matrix_0 and matrix_1
// @param matrix_0 the first matrix to be multiplied
// @param matrix_1 the second matrix to be multiplied
// @return the result of the matrix multiplication
KINLINE mat4 mat4_mul(mat4 matrix_0, mat4 matrix_1) {
    mat4 out_matrix = mat4_identity();  // create a mat 4 identity - look at top of mat 4 section - identity matrix

    // pointer magic to make this work
    const f32* m1_ptr = matrix_0.data;  // initialize a pointer to matrix 0 array, store in m1_ptr
    const f32* m2_ptr = matrix_1.data;  // initialize a pointer to matrix 1 array, store in m2_ptr
    f32* dst_ptr = out_matrix.data;     // initialize a pointer to the out matrix array, store in dst_ptr

    for (i32 i = 0; i < 4; ++i) {      // iterate through the rows of the resulting matrix
        for (i32 j = 0; j < 4; ++j) {  // iterate through the collumns of the resulting matrix
            *dst_ptr =                 // this all gives us the dot product of i index row of matrix 0 with the j index collumn of matrix 1, and stores it in the (i, j) position of of the resulting arraay - pointer means this pushes into the actual array
                m1_ptr[0] * m2_ptr[0 + j] +
                m1_ptr[1] * m2_ptr[4 + j] +
                m1_ptr[2] * m2_ptr[8 + j] +
                m1_ptr[3] * m2_ptr[12 + j];
            dst_ptr++;  // increment so it points to the next element in the resulting array the (i, j) - this is just the pointer to the data
        }
        m1_ptr += 4;  // increment to move to the next row
    }
    return out_matrix;
}

// need to learn more about this stuff
// @brief creates and returns an orthographic projection matrix. typically used to render flat or 2d scenes
// @param left the left side of the view frustum.
// @param right the right side of the view frustum.
// @param bottom the bottom side of the view frustum.
// @param top the top side of the view frustum.
// @param near_clip the near clipping plane distance
// @param far_clip the far clipping plane distance
// @result a new orthographic matrix

KINLINE mat4 mat4_orthographic(f32 left, f32 right, f32 bottom, f32 top, f32 near_clip, f32 far_clip) {
    mat4 out_matrix = mat4_identity();  // create a new identity matrix

    // these calculate reciprocals of the differences between the respective pairs bottom-top, and near-far boundaries of the view frustum. used for scaling and translation operations
    f32 lr = 1.0f / (left - right);          // left right
    f32 bt = 1.0f / (bottom - top);          // bottom top
    f32 nf = 1.0f / (near_clip - far_clip);  // near far

    //  These values affect the scaling and projection of the geometry when multiplied by the vertices - negative values mirror along those axis
    out_matrix.data[0] = -2.0f * lr;
    out_matrix.data[5] = -2.0f * bt;
    out_matrix.data[10] = 2.0f * nf;

    // These lines assign specific values to the translation components of the out_matrix. They are calculated based on the sums of the respective boundary values and then scaled by the reciprocals calculated earlier.
    out_matrix.data[12] = (left + right) * lr;
    out_matrix.data[13] = (top + bottom) * bt;
    out_matrix.data[14] = (far_clip + near_clip) * nf;
    return out_matrix;
}

// @brief creates and returns a perspective matrix. typically used to render 3d scenes
// @param fov_radians the field of view in radians
// @param aspect_ratio the aspect ratio (width divided by height)
// @param near_clip the near clipping plane distance
// @param far_clip the far clipping plane distance
// @return a new perspective matrix
KINLINE mat4 mat4_perspective(f32 fov_radians, f32 aspect_ratio, f32 near_clip, f32 far_clip) {
    f32 half_tan_fov = ktan(fov_radians * 0.5f);                                      // create a float with the value of the tangent of (fov radians divided in half)
    mat4 out_matrix;                                                                  // create a new 4 x 4 matrix, name out
    kzero_memory(out_matrix.data, sizeof(f32) * 16);                                  // zero out a block of memory, use out matrix data to get the adress, and use the size of a f32 times 16 - for the number of elements
    out_matrix.data[0] = 1.0f / (aspect_ratio * half_tan_fov);                        // set index 0 to 1 divided by(aspect ratio times the tangent of (fov radians divided in half))
    out_matrix.data[5] = 1.0f / half_tan_fov;                                         // set index 5 to 1 divided by the tangent of (fov radians divided in half)
    out_matrix.data[10] = -((far_clip + near_clip) / (far_clip - near_clip));         // set index 10 to negative ((far_clip + near_clip) divided by (far_clip - near_clip))
    out_matrix.data[11] = -1.0f;                                                      // set index 11 to -1
    out_matrix.data[14] = -((2.0f * far_clip * near_clip) / (far_clip - near_clip));  // set index 14 to negative ((2 times farclip times near clip) divided by (far clip - near clip))
    return out_matrix;
}

// a way to get a camera to follow an object. again i need to look this stuff up
// @brief creates and returns a look-at matrix, or a matrix looking at target from the perpective of position
// @param position the position of the matrix
// @param target the position to "look at"
// @param up the up vector
// @return a matrix looking at target from the perspective of position
KINLINE mat4 mat4_look_at(vec3 position, vec3 target, vec3 up) {
    mat4 out_matrix;                   // create a 4 x 4 matrix called out matrix
    vec3 z_axis;                       // create a vec3 called z axis
    z_axis.x = target.x - position.x;  // set x to target x - position x
    z_axis.y = target.y - position.y;  // set y to target y - position y
    z_axis.z = target.z - position.z;  // set z to target z - position z

    z_axis = vec3_normalized(z_axis);                       // normalize the vec3 z axis
    vec3 x_axis = vec3_normalized(vec3_cross(z_axis, up));  // create a vec 3 x axis, with a normalized value of a vec 3 cross between z axis and up vec3
    vec3 y_axis = vec3_cross(x_axis, z_axis);               // create a vec3 y axis with the value of a vec3 cross between the x axis and the z axis

    // x values - row 1
    out_matrix.data[0] = x_axis.x;   // 1st collumn comes from x axis
    out_matrix.data[1] = y_axis.x;   // 2nd collumn comes from x axis
    out_matrix.data[2] = -z_axis.x;  // 3rd collumn comes from z axis neg
    out_matrix.data[3] = 0;          // fourth collulmn is zero
    // y values - row2
    out_matrix.data[4] = x_axis.y;   // 1st collumn comes from x axis
    out_matrix.data[5] = y_axis.y;   // 2nd collumn comes from x axis
    out_matrix.data[6] = -z_axis.y;  // 3rd collumn comes from z axis neg
    out_matrix.data[7] = 0;          // fourth collulmn is zero
    // z values - row3
    out_matrix.data[8] = x_axis.z;    // 1st collumn comes from x axis
    out_matrix.data[9] = y_axis.z;    // 2nd collumn comes from x axis
    out_matrix.data[10] = -z_axis.z;  // 3rd collumn comes from z axis neg
    out_matrix.data[11] = 0;          // fourth collulmn is zero
    //         - row4
    out_matrix.data[12] = -vec3_dot(x_axis, position);  // 1st collumn comes from the dot product of x axis and position neg
    out_matrix.data[13] = -vec3_dot(y_axis, position);  // 2nd collumn comes from the dot product of x axis and position neg
    out_matrix.data[14] = vec3_dot(z_axis, position);   // 3rd collumn comes from the dot product of z axis and position
    out_matrix.data[15] = 1.0f;                         // fourth collulmn is one

    return out_matrix;
}

// @brief returns a transposed copy of the provided matrix (row->columns) all of the rows become columns
// @param matrix the matrix to be transposed
// @return a transposed copy of the provided matrix
KINLINE mat4 mat4_transposed(mat4 matrix) {
    mat4 out_matrix = mat4_identity();  // create an identity matrix
    // row one to column one
    out_matrix.data[0] = matrix.data[0];
    out_matrix.data[1] = matrix.data[4];
    out_matrix.data[2] = matrix.data[8];
    out_matrix.data[3] = matrix.data[12];
    // row 2 to column 2
    out_matrix.data[4] = matrix.data[1];
    out_matrix.data[5] = matrix.data[5];
    out_matrix.data[6] = matrix.data[9];
    out_matrix.data[7] = matrix.data[13];
    // row 3 to column 3
    out_matrix.data[8] = matrix.data[2];
    out_matrix.data[9] = matrix.data[6];
    out_matrix.data[10] = matrix.data[10];
    out_matrix.data[11] = matrix.data[14];
    // row 4 to column 4
    out_matrix.data[12] = matrix.data[3];
    out_matrix.data[13] = matrix.data[7];
    out_matrix.data[14] = matrix.data[11];
    out_matrix.data[15] = matrix.data[15];

    return out_matrix;
}

// he barely goes over this one so i definately need to look it up -- basically it reverses the operations of the matrix
// so if the matrix is to display a 3d in 2d space, the inverse would display the 2d object in 3d space
// @brief Creates and returns an inverse of the provided matrix.
// @param matrix The matrix to be inverted.
// @return A inverted copy of the provided matrix.
KINLINE mat4 mat4_inverse(mat4 matrix) {
    const f32* m = matrix.data;

    f32 t0 = m[10] * m[15];
    f32 t1 = m[14] * m[11];
    f32 t2 = m[6] * m[15];
    f32 t3 = m[14] * m[7];
    f32 t4 = m[6] * m[11];
    f32 t5 = m[10] * m[7];
    f32 t6 = m[2] * m[15];
    f32 t7 = m[14] * m[3];
    f32 t8 = m[2] * m[11];
    f32 t9 = m[10] * m[3];
    f32 t10 = m[2] * m[7];
    f32 t11 = m[6] * m[3];
    f32 t12 = m[8] * m[13];
    f32 t13 = m[12] * m[9];
    f32 t14 = m[4] * m[13];
    f32 t15 = m[12] * m[5];
    f32 t16 = m[4] * m[9];
    f32 t17 = m[8] * m[5];
    f32 t18 = m[0] * m[13];
    f32 t19 = m[12] * m[1];
    f32 t20 = m[0] * m[9];
    f32 t21 = m[8] * m[1];
    f32 t22 = m[0] * m[5];
    f32 t23 = m[4] * m[1];

    mat4 out_matrix;
    f32* o = out_matrix.data;

    o[0] = (t0 * m[5] + t3 * m[9] + t4 * m[13]) - (t1 * m[5] + t2 * m[9] + t5 * m[13]);
    o[1] = (t1 * m[1] + t6 * m[9] + t9 * m[13]) - (t0 * m[1] + t7 * m[9] + t8 * m[13]);
    o[2] = (t2 * m[1] + t7 * m[5] + t10 * m[13]) - (t3 * m[1] + t6 * m[5] + t11 * m[13]);
    o[3] = (t5 * m[1] + t8 * m[5] + t11 * m[9]) - (t4 * m[1] + t9 * m[5] + t10 * m[9]);

    f32 d = 1.0f / (m[0] * o[0] + m[4] * o[1] + m[8] * o[2] + m[12] * o[3]);

    o[0] = d * o[0];
    o[1] = d * o[1];
    o[2] = d * o[2];
    o[3] = d * o[3];
    o[4] = d * ((t1 * m[4] + t2 * m[8] + t5 * m[12]) - (t0 * m[4] + t3 * m[8] + t4 * m[12]));
    o[5] = d * ((t0 * m[0] + t7 * m[8] + t8 * m[12]) - (t1 * m[0] + t6 * m[8] + t9 * m[12]));
    o[6] = d * ((t3 * m[0] + t6 * m[4] + t11 * m[12]) - (t2 * m[0] + t7 * m[4] + t10 * m[12]));
    o[7] = d * ((t4 * m[0] + t9 * m[4] + t10 * m[8]) - (t5 * m[0] + t8 * m[4] + t11 * m[8]));
    o[8] = d * ((t12 * m[7] + t15 * m[11] + t16 * m[15]) - (t13 * m[7] + t14 * m[11] + t17 * m[15]));
    o[9] = d * ((t13 * m[3] + t18 * m[11] + t21 * m[15]) - (t12 * m[3] + t19 * m[11] + t20 * m[15]));
    o[10] = d * ((t14 * m[3] + t19 * m[7] + t22 * m[15]) - (t15 * m[3] + t18 * m[7] + t23 * m[15]));
    o[11] = d * ((t17 * m[3] + t20 * m[7] + t23 * m[11]) - (t16 * m[3] + t21 * m[7] + t22 * m[11]));
    o[12] = d * ((t14 * m[10] + t17 * m[14] + t13 * m[6]) - (t16 * m[14] + t12 * m[6] + t15 * m[10]));
    o[13] = d * ((t20 * m[14] + t12 * m[2] + t19 * m[10]) - (t18 * m[10] + t21 * m[14] + t13 * m[2]));
    o[14] = d * ((t18 * m[6] + t23 * m[14] + t15 * m[2]) - (t22 * m[14] + t14 * m[2] + t19 * m[6]));
    o[15] = d * ((t22 * m[10] + t16 * m[2] + t21 * m[6]) - (t20 * m[6] + t23 * m[10] + t17 * m[2]));

    return out_matrix;
}

// some more commonly used functions
// translation function -- creates a translation matrix -- this seems like a much easier place to start learning these things
KINLINE mat4 mat4_translation(vec3 position) {
    mat4 out_matrix = mat4_identity();  // create a new identity matrix
    out_matrix.data[12] = position.x;   // pass in positions x to in index 12
    out_matrix.data[13] = position.y;   // pass in positions y to in index 13
    out_matrix.data[14] = position.z;   // pass in positions z to in index 14
    return out_matrix;
}

// @brief returns a scale matrix using the provided scale.
// @param scale the 3 component scale.
// @return a scale matrix
KINLINE mat4 mat4_scale(vec3 scale) {
    mat4 out_matrix = mat4_identity();  // create a new identity matrix
    out_matrix.data[0] = scale.x;       // pass in scale x to index 0
    out_matrix.data[5] = scale.y;       // pass in scale y to index 5
    out_matrix.data[10] = scale.z;      // pass in scale z to index 10
    return out_matrix;
}

// euler functions - dont want to use all the time but can come in handy
// a matrix for rotating along the x axis
KINLINE mat4 mat4_euler_x(f32 angle_radians) {
    mat4 out_matrix = mat4_identity();  // create a new identity matrix
    f32 c = kcos(angle_radians);        // get the cosine value of the angle store in c
    f32 s = ksin(angle_radians);        // get the sine value of the angle store in s

    out_matrix.data[5] = c;   // set index 5 to c
    out_matrix.data[6] = s;   // index 6 to s
    out_matrix.data[9] = -s;  // index 9 to negative s
    out_matrix.data[10] = c;  // and index 10 to c
    return out_matrix;
}

// a matrix for rotating along the y axis
KINLINE mat4 mat4_euler_y(f32 angle_radians) {
    mat4 out_matrix = mat4_identity();  // create a new identity matrix
    f32 c = kcos(angle_radians);        // get the cosine value of the angle store in c
    f32 s = ksin(angle_radians);        // get the sine value of the angle store in s

    out_matrix.data[0] = c;   // set index 0 to c
    out_matrix.data[2] = -s;  // index 2 to negative s
    out_matrix.data[8] = s;   // index 8 to s
    out_matrix.data[10] = c;  // and index 10 to c
    return out_matrix;
}

// a matrix for rotating along the z axis
KINLINE mat4 mat4_euler_z(f32 angle_radians) {
    mat4 out_matrix = mat4_identity();  // create a new identity matrix
    f32 c = kcos(angle_radians);        // get the cosine value of the angle store in c
    f32 s = ksin(angle_radians);        // get the sine value of the angle store in s

    out_matrix.data[0] = c;   // set index 0 to c
    out_matrix.data[1] = s;   // index 1 to s
    out_matrix.data[4] = -s;  // index 4 to negative s
    out_matrix.data[5] = c;   // and index 5 to c
    return out_matrix;
}

// matrix for rotating on all axi
KINLINE mat4 mat4_euler_xyz(f32 x_radians, f32 y_radians, f32 z_radians) {
    mat4 rx = mat4_euler_x(x_radians);      // create an x axis euler matrix
    mat4 ry = mat4_euler_y(y_radians);      // create an y axis euler matrix
    mat4 rz = mat4_euler_z(z_radians);      // create an z axis euler matrix
    mat4 out_matrix = mat4_mul(rx, ry);     // create the result matrix with the value being a multiplication of the x and y axis euler matrices
    out_matrix = mat4_mul(out_matrix, rz);  // multiply the result matrix by the z axis euler matrix
    return out_matrix;
}

// @brief returns a foreward vector relative to the provided matrix - for moving foreward and such based on the position and rotation the object is currently at - gives what the forward value is
// @param matrix the matrix from which to base the vector
// @return a 3-component directional vector
KINLINE vec3 mat4_forward(mat4 matrix) {
    vec3 forward;                  // create a vec3 forward
    forward.x = -matrix.data[2];   // get x value from negative matrix at index 2
    forward.y = -matrix.data[6];   // get y value from negative matrix at index 6
    forward.z = -matrix.data[10];  // get z value from negative matrix at index 10
    vec3_normalize(&forward);      // normalize the values recieved
    return forward;
}

// @brief returns a backward vector relative to the provided matrix - for moving backward and such based on the position and rotation the object is currently at - gives what the backward value is
// @param matrix the matrix from which to base the vector
// @return a 3-component directional vector
KINLINE vec3 mat4_backward(mat4 matrix) {
    vec3 backward;                 // create a vec3 backward
    backward.x = matrix.data[2];   // get x value from matrix at index 2
    backward.y = matrix.data[6];   // get y value from matrix at index 6
    backward.z = matrix.data[10];  // get z value from matrix at index 10
    vec3_normalize(&backward);     // normalize the values recieved
    return backward;
}

// @brief returns a upward vector relative to the provided matrix - for moving upward and such based on the position and rotation the object is currently at - gives what the upward value is
// @param matrix the matrix from which to base the vector
// @return a 3-component directional vector
KINLINE vec3 mat4_upward(mat4 matrix) {
    vec3 upward;                // create a vec3 upward
    upward.x = matrix.data[1];  // get x value from matrix at index 2
    upward.y = matrix.data[5];  // get y value from matrix at index 6
    upward.z = matrix.data[9];  // get z value from matrix at index 10
    vec3_normalize(&upward);    // normalize the values recieved
    return upward;
}

// @brief returns a downward vector relative to the provided matrix - for moving downward and such based on the position and rotation the object is currently at - gives what the downward value is
// @param matrix the matrix from which to base the vector
// @return a 3-component directional vector
KINLINE vec3 mat4_downward(mat4 matrix) {
    vec3 downward;                 // create a vec3 downward
    downward.x = -matrix.data[1];  // get x value from negative matrix at index 2
    downward.y = -matrix.data[5];  // get y value from negative matrix at index 6
    downward.z = -matrix.data[9];  // get z value from negative matrix at index 10
    vec3_normalize(&downward);     // normalize the values recieved
    return downward;
}

// @brief returns a left vector relative to the provided matrix - for moving left and such based on the position and rotation the object is currently at - gives what the left value is
// @param matrix the matrix from which to base the vector
// @return a 3-component directional vector
KINLINE vec3 mat4_left(mat4 matrix) {
    vec3 left;                 // create a vec3 left
    left.x = -matrix.data[0];  // get x value from negative matrix at index 2
    left.y = -matrix.data[4];  // get y value from negative matrix at index 6
    left.z = -matrix.data[8];  // get z value from negative matrix at index 10
    vec3_normalize(&left);     // normalize the values recieved
    return left;
}

// @brief returns a right vector relative to the provided matrix - for moving right and such based on the position and rotation the object is currently at - gives what the right value is
// @param matrix the matrix from which to base the vector
// @return a 3-component directional vector
KINLINE vec3 mat4_right(mat4 matrix) {
    vec3 right;                 // create a vec3 right
    right.x = -matrix.data[0];  // get x value from negative matrix at index 2
    right.y = -matrix.data[4];  // get y value from negative matrix at index 6
    right.z = -matrix.data[8];  // get z value from negative matrix at index 10
    vec3_normalize(&right);     // normalize the values recieved
    return right;
}

// quaternions ------------------------------------------------------------------------------------------------------------------------------------------------

// create an identiy quaternion - the default quaternion
KINLINE quat quat_identify() {
    return (quat){0, 0, 0, 1.0f};  // create a vec4 with all zeros except a 1 in the w index
}

// caluculate a quat normal --
KINLINE f32 quat_normal(quat q) {
    return ksqrt(    // return the sqare root of
        q.x * q.x +  // x squared
        q.y * q.y +  // plus y squared
        q.z * q.z +  // plus z squared
        q.w * q.w);  // plus w squared
}

// normalize a quaternion
KINLINE quat quat_normalize(quat q) {
    f32 normal = quat_normal(q);  // define a f32 with the value of a quat normal of q
    return (quat){
        q.x / normal,   // normal x divided by normal
        q.y / normal,   // normal y divided by normal
        q.z / normal,   // normal z divided by normal
        q.w / normal};  // normal w divided by normal
}

// calculate a quat conjugate - flip the x, y and z axi
KINLINE quat quat_conjugate(quat q) {
    return (quat){
        -q.x,
        -q.y,
        -q.z,
        q.w};
}

// calculate a quat inverse - a normalized quat conjugate
KINLINE quat quat_inverse(quat q) {
    return quat_normalize(quat_conjugate(q));
}

// multiply 2 quaternions - another really confusing one that i need to study -- suposedly this is the only way to multiply 2 quaternions together
KINLINE quat quat_mul(quat q_0, quat q_1) {
    quat out_quaternion;

    out_quaternion.x = q_0.x * q_1.w +
                       q_0.y * q_1.z -
                       q_0.z * q_1.y +
                       q_0.w * q_1.x;

    out_quaternion.y = -q_0.x * q_1.z +
                       q_0.y * q_1.w +
                       q_0.z * q_1.x +
                       q_0.w * q_1.y;

    out_quaternion.z = q_0.x * q_1.y -
                       q_0.y * q_1.x +
                       q_0.z * q_1.w +
                       q_0.w * q_1.z;

    out_quaternion.w = -q_0.x * q_1.x -
                       q_0.y * q_1.y -
                       q_0.z * q_1.z +
                       q_0.w * q_1.w;

    return out_quaternion;
}

// get the dot product between quaternions
KINLINE f32 quat_dot(quat q_0, quat q_1) {
    return q_0.x * q_1.x +  // multiply xs
           q_0.y * q_1.y +  // plus multiply ys
           q_0.z * q_1.z +  // plus multiply zs
           q_0.w * q_1.w;   // plus multiply ws
}

// output a quaternion rotation to a matrix
// he got this one from a stack over throw thread, the link is in the video (#023) at 25 14 if i need to reference it
KINLINE mat4 quat_to_mat4(quat q) {
    mat4 out_matrix = mat4_identity();  // create a new resulting identity matrix

    quat n = quat_normalize(q);  // normalize q and store the result in n

    out_matrix.data[0] = 1.0f - 2.0f * n.y * n.y - 2.0f * n.z * n.z;  // index 0 is the result of = 1 - (2 * y * y) - (2 * z * z)
    out_matrix.data[1] = 2.0f * n.x * n.y - 2.0f * n.z * n.w;         // index 1 is the result of = (2 * x * y) - (2 * z * w)
    out_matrix.data[2] = 2.0f * n.x * n.z + 2.0f * n.y * n.w;         // index 2 is the result of = (2 * x * z) + (2 * y * w)

    out_matrix.data[4] = 2.0f * n.x * n.y + 2.0f * n.z * n.w;         // index 4 is the result of = (2 * x * y) + (2 * z * w)
    out_matrix.data[5] = 1.0f - 2.0f * n.x * n.x - 2.0f * n.z * n.z;  // index 5 is the result of = 1 - (2 * x * x) - (2 * z * z)
    out_matrix.data[6] = 2.0f * n.y * n.z - 2.0f * n.x * n.w;         // index 6 is the result of = (2 * y * z) - (2 * x * w)

    out_matrix.data[8] = 2.0f * n.x * n.z - 2.0f * n.y * n.w;          // index 8 is the result of = (2 * x * z) - (2 * y * w)
    out_matrix.data[9] = 2.0f * n.y * n.z + 2.0f * n.x * n.w;          // index 9 is the result of = (2 * y * z) + (2 * x * w)
    out_matrix.data[10] = 1.0f - 2.0f * n.x * n.x - 2.0f * n.y * n.y;  // index 10 is the result of = 1 - (2 * x * x) - (2 * y * y)

    return out_matrix;
}

// need to look this one up as well
// calculates a rotation matrix based on the quaternion and the passed in center point.
KINLINE mat4 quat_to_rotation_matrix(quat q, vec3 center) {
    mat4 out_matrix;  // create a 4 x 4 matrix

    f32* o = out_matrix.data;  // create a poiter to the out matrix data and store it in o
    o[0] = (q.x * q.x) - (q.y * q.y) - (q.z * q.z) + (q.w * q.w);
    o[1] = 2.0f * ((q.x * q.y) + (q.z * q.w));
    o[2] = 2.0f * ((q.x * q.z) - (q.y * q.w));
    o[3] = center.x - center.x * o[0] - center.y * o[1] - center.z * o[2];

    o[4] = 2.0f * ((q.x * q.y) - (q.z * q.w));
    o[5] = -(q.x * q.x) + (q.y * q.y) - (q.z * q.z) + (q.w * q.w);
    o[6] = 2.0f * ((q.y * q.z) + (q.x * q.w));
    o[7] = center.y - center.x * o[4] - center.y * o[5] - center.z * o[6];

    o[8] = 2.0f * ((q.x * q.z) + (q.y * q.w));
    o[9] = 2.0f * ((q.y * q.z) - (q.x * q.w));
    o[10] = -(q.x * q.x) - (q.y * q.y) + (q.z * q.z) + (q.w * q.w);
    o[11] = center.z - center.x * o[8] - center.y * o[9] - center.z * o[10];

    o[12] = 0.0f;
    o[13] = 0.0f;
    o[14] = 0.0f;
    o[15] = 1.0f;
    return out_matrix;
}

// essencialy a eular rotation - but converted into a quaternion
KINLINE quat quat_from_axis_angle(vec3 axis, f32 angle, b8 normalize) {
    const f32 half_angle = 0.5f * angle;  // define a f32 with the value of half of the angle
    f32 s = ksin(half_angle);             // define a f32 with the value of the sin value of half angle
    f32 c = kcos(half_angle);             // define a f32 with the value of the cosin value of half angle

    quat q = (quat){s * axis.x, s * axis.y, s * axis.z, c};  // define a resulting quaternion with the values of s * x, s * y, s * z, and c
    if (normalize) {                                         // if normalize set to true
        return quat_normalize(q);                            // normalize the resulting quat
    }
    return q;
}

// he doesnt really explain this one either - spherical linear interpolation - it finds the rotation that is inbetween the 2 rotations and finds what is is at a specific percentage
KINLINE quat quat_slerp(quat q_0, quat q_1, f32 percentage) {
    quat out_quaternion;  // define the resulting quaternion
    // source: https://en.wikipedia.org/wiki/Slerp  - video #023 at 27 15 -- read this article
    // only unit quaternions are valid rotations
    // normalize to avoid undefined behavior store in v variables
    quat v0 = quat_normalize(q_0);
    quat v1 = quat_normalize(q_1);

    // compute the cosine of the angle between the two vectors
    f32 dot = quat_dot(v0, v1);  // get the dot product of the normalized quats and stor in dot

    // if the dot product is negative, slerp wont take the shorter path. note that v1 and -v1 are equivalent when
    // the negative is applied to all four components. fix by reversing one quaternion
    if (dot < 0.0f) {  // if dot is negative - change the vallue of everything to positive
        v1.x = -v1.x;
        v1.y = -v1.y;
        v1.z = -v1.z;
        v1.w = -v1.w;
        dot = -dot;
    }

    const f32 DOT_THRESHOLD = 0.9995f;
    if (dot > DOT_THRESHOLD) {
        // if the inputs are too close for comfort, linearly interpolate and normalize the result
        out_quaternion = (quat){
            v0.x + ((v1.x - v0.x) * percentage),
            v0.y + ((v1.y - v0.y) * percentage),
            v0.z + ((v1.z - v0.z) * percentage),
            v0.w + ((v1.w - v0.w) * percentage)};

        return quat_normalize(out_quaternion);
    }

    // since dot is in range [0, dot_threshold], acos is safe
    f32 theta_0 = kacos(dot);          // theta_0 = angle between input vectors
    f32 theta = theta_0 * percentage;  // theta = angle between v0 and result
    f32 sin_theta = ksin(theta);       // compute this value only once
    f32 sin_theta_0 = ksin(theta_0);   // compute this value only once

    f32 s0 = kcos(theta) - dot * sin_theta / sin_theta_0;  // sin(theta_0 - theta) / sin(theta_0)
    f32 s1 = sin_theta / sin_theta_0;

    return (quat){
        (v0.x * s0) + (v1.x * s1),
        (v0.y * s0) + (v1.y * s1),
        (v0.z * s0) + (v1.z * s1),
        (v0.w * s0) + (v1.w * s1)};
}

// @brief converts provided degrees to radians
// @param degrees the degrees to be converted
// @retun the amount in radians

KINLINE f32 deg_to_rad(f32 degrees) {
    return degrees * K_DEG2RAD_MULTIPLIER;
}

// @brief converts provided radians to degrees
// @param radians the radians to be converted
// @return the amount in degrees
KINLINE f32 rad_to_deg(f32 radians) {
    return radians * K_RAD2DEG_MULTIPLIER;
}