#include "math_types.h"

// @brief creates and returns a new transform, using a zero vector for position, identity
// quaternion for rotation, and a one vector for scale. also has a null parent.
// marked dirty by default
KAPI transform transform_create();

// @brief creates a transform from a given position. uses a zero rotation and a one scale
// @param position the position to be used.
// @return a new transform
KAPI transform transform_from_position(vec3 position);

// @brief creates a transform from a given rotation. uses a zero position and a one scale
// @param rotation the rotation to be used.
// @return a new transform
KAPI transform transform_from_rotation(quat rotation);

// @brief creates a transform from a given position and rotation. uses a one scale
// @param position the position to be used.
// @param rotation the rotation to be used.
// @return a new transform
KAPI transform transform_from_position_rotation(vec3 position, quat rotation);

// @brief creates a transform from a given position, rotation and scale
// @param position the position to be used.
// @param rotation the rotation to be used.
// @param scale the scale to be used
// @return a new transform
KAPI transform transform_from_position_rotation_scale(vec3 position, quat rotation, vec3 scale);

// @brief returns a pointer to the provided transforms parent
// @param t a pointer to the transform whose parent to retrieve
// @return a pointer to the parent transform
KAPI transform* transform_get_parent(transform* t);

// @brief sets the parent of the provided transform
// @param t a pointer to the transform whose parent will be set
// @param parent a pointer to the parent transform
KAPI void transform_set_parent(transform* t, transform* parent);

// @brief returns the position of the given transform
// @param t a constant pointer whose position to get.
// @return a copy of the position.
KAPI vec3 transform_get_position(const transform* t);

// @brief sets the position of the given transform
// @param t a pointer to the transform to be updated
// @param position the position to be set
KAPI void transform_set_position(transform* t, vec3 position);

// @brief applies a translation to the given transform. not the same as setting
// @param t a pointer to the transform to be updated
// @param translation the translation to be applied
KAPI void transform_translate(transform* t, vec3 translation);

// @brief returns the rotation of the given transform
// @param t a constant pointer whose rotation to get.
// @return a copy of the rotation
KAPI quat transform_get_rotation(const transform* t);

// @brief sets the rotation of the given transform
// @param t a pointer to the transform to be updated
// @param rotation the rotation to be set
KAPI void transform_set_rotation(transform* t, quat rotation);

// @brief applies a rotation to the given transform. not the same as setting
// @param t a pointer to the transform to be updated
// @param rotation the rotation to be applied
KAPI void transform_rotate(transform* t, quat rotation);

// @brief returns the scale of the given transform
// @param t a constant pointer whose scale to get.
// @return a copy of the scale
KAPI vec3 transform_get_scale(const transform* t);

// @brief sets the scale of the given transform
// @param t a pointer to the transform to be updated
// @param scale the scale to be set
KAPI void transform_set_scale(transform* t, vec3 scale);

// @brief applies a scale to the given transform. not the same as setting
// @param t a pointer to the transform to be updated
// @param scale the scale to be applied
KAPI void transform_scale(transform* t, vec3 scale);

// @brief sets the position and rotation of the given transform
// @param t a pointer to the transform to be updated
// @param position the position to be set
// @param rotation the rotation to be set
KAPI void transform_set_position_rotation(transform* t, vec3 position, quat rotation);

// @brief sets the position, rotation and scale of the given transform
// @param t a pointer to the transform to be updated
// @param position the position to be set
// @param rotation the rotation to be set
// @param scale the scale to be set
KAPI void transform_set_position_rotation_scale(transform* t, vec3 position, quat rotation, vec3 scale);

// @brief applies a translation and rotation to the given transform. not the same as setting
// @param t a pointer to the transform to be updated
// @param translation the translation to be applied
// @param rotation the rotation to be applied
KAPI void transform_translate_rotate(transform* t, vec3 translation, quat rotation);

// @brief retrieves the local transformation matrix from the provided transform. automatically recalculates
// the matrix if it is dirty. otherwise, the already calculated one is returned
// @param t a pointer to the transform whose matrix to retrieve.
// @return a copy of the local transformation matrix
KAPI mat4 transform_get_local(transform* t);

// @brief obtains the world matrix of the given transform by examining its parent (if there is one)
// and multiplying it against the local matrix
// @param t a pointer to the transform whose world matrix to retrieve
// @return a copy of the world matrix
KAPI mat4 transform_get_world(transform* t);