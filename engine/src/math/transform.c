#include "transform.h"

#include "kmath.h"

// @brief creates and returns a new transform, using a zero vector for position, identity
// quaternion for rotation, and a one vector for scale. also has a null parent.
// marked dirty by default
transform transform_create() {
    transform t;
    transform_set_position_rotation_scale(&t, vec3_zero(), quat_identity(), vec3_one());
    t.local = mat4_identity();
    t.parent = 0;
    return t;
}

// @brief creates a transform from a given position. uses a zero rotation and a one scale
// @param position the position to be used.
// @return a new transform
transform transform_from_position(vec3 position) {
    transform t;
    transform_set_position_rotation_scale(&t, position, quat_identity(), vec3_one());
    t.local = mat4_identity();
    t.parent = 0;
    return t;
}

// @brief creates a transform from a given rotation. uses a zero position and a one scale
// @param rotation the rotation to be used.
// @return a new transform
transform transform_from_rotation(quat rotation) {
    transform t;
    transform_set_position_rotation_scale(&t, vec3_zero(), rotation, vec3_one());
    t.local = mat4_identity();
    t.parent = 0;
    return t;
}

// @brief creates a transform from a given position and rotation. uses a one scale
// @param position the position to be used.
// @param rotation the rotation to be used.
// @return a new transform
transform transform_from_position_rotation(vec3 position, quat rotation) {
    transform t;
    transform_set_position_rotation_scale(&t, position, rotation, vec3_one());
    t.local = mat4_identity();
    t.parent = 0;
    return t;
}

// @brief creates a transform from a given position, rotation and scale
// @param position the position to be used.
// @param rotation the rotation to be used.
// @param scale the scale to be used
// @return a new transform
transform transform_from_position_rotation_scale(vec3 position, quat rotation, vec3 scale) {
    transform t;
    transform_set_position_rotation_scale(&t, position, rotation, scale);
    t.local = mat4_identity();
    t.parent = 0;
    return t;
}

// @brief returns a pointer to the provided transforms parent
// @param t a pointer to the transform whose parent to retrieve
// @return a pointer to the parent transform
transform* transform_get_parent(transform* t) {
    if (!t) {
        return 0;
    }
    return t->parent;
}

// @brief sets the parent of the provided transform
// @param t a pointer to the transform whose parent will be set
// @param parent a pointer to the parent transform
void transform_set_parent(transform* t, transform* parent) {
    if (t) {
        t->parent = parent;
    }
}

// @brief returns the position of the given transform
// @param t a constant pointer whose position to get.
// @return a copy of the position.
vec3 transform_get_position(const transform* t) {
    return t->position;
}

// @brief sets the position of the given transform
// @param t a pointer to the transform to be updated
// @param position the position to be set
void transform_set_position(transform* t, vec3 position) {
    t->position = position;
    t->is_dirty = true;
}

// @brief applies a translation to the given transform. not the same as setting
// @param t a pointer to the transform to be updated
// @param translation the translation to be applied
void transform_translate(transform* t, vec3 translation) {
    t->position = vec3_add(t->position, translation);
    t->is_dirty = true;
}

// @brief returns the rotation of the given transform
// @param t a constant pointer whose rotation to get.
// @return a copy of the rotation
quat transform_get_rotation(const transform* t) {
    return t->rotation;
}

// @brief sets the rotation of the given transform
// @param t a pointer to the transform to be updated
// @param rotation the rotation to be set
void transform_set_rotation(transform* t, quat rotation) {
    t->rotation = rotation;
    t->is_dirty = true;
}

// @brief applies a rotation to the given transform. not the same as setting
// @param t a pointer to the transform to be updated
// @param rotation the rotation to be applied
void transform_rotate(transform* t, quat rotation) {
    t->rotation = quat_mul(t->rotation, rotation);
    t->is_dirty = true;
}

// @brief returns the scale of the given transform
// @param t a constant pointer whose scale to get.
// @return a copy of the scale
vec3 transform_get_scale(const transform* t) {
    return t->scale;
}

// @brief sets the scale of the given transform
// @param t a pointer to the transform to be updated
// @param scale the scale to be set
void transform_set_scale(transform* t, vec3 scale) {
    t->scale = scale;
    t->is_dirty = true;
}

// @brief applies a scale to the given transform. not the same as setting
// @param t a pointer to the transform to be updated
// @param scale the scale to be applied
void transform_scale(transform* t, vec3 scale) {
    t->scale = vec3_mul(t->scale, scale);
    t->is_dirty = true;
}

// @brief sets the position and rotation of the given transform
// @param t a pointer to the transform to be updated
// @param position the position to be set
// @param rotation the rotation to be set
void transform_set_position_rotation(transform* t, vec3 position, quat rotation) {
    t->position = position;
    t->rotation = rotation;
    t->is_dirty = true;
}

// @brief sets the position, rotation and scale of the given transform
// @param t a pointer to the transform to be updated
// @param position the position to be set
// @param rotation the rotation to be set
// @param scale the scale to be set
void transform_set_position_rotation_scale(transform* t, vec3 position, quat rotation, vec3 scale) {
    t->position = position;
    t->rotation = rotation;
    t->scale = scale;
    t->is_dirty = true;
}

// @brief applies a translation and rotation to the given transform. not the same as setting
// @param t a pointer to the transform to be updated
// @param translation the translation to be applied
// @param rotation the rotation to be applied
void transform_translate_rotate(transform* t, vec3 translation, quat rotation) {
    t->position = vec3_add(t->position, translation);
    t->rotation = quat_mul(t->rotation, rotation);
    t->is_dirty = true;
}

// @brief retrieves the local transformation matrix from the provided transform. automatically recalculates
// the matrix if it is dirty. otherwise, the already calculated one is returned
// @param t a pointer to the transform whose matrix to retrieve.
// @return a copy of the local transformation matrix
mat4 transform_get_local(transform* t) {
    if (t) {
        if (t->is_dirty) {
            mat4 tr = mat4_mul(quat_to_mat4(t->rotation), mat4_translation(t->position));
            tr = mat4_mul(mat4_scale(t->scale), tr);
            t->local = tr;
            t->is_dirty = false;
        }

        return t->local;
    }
    // if it fails return a default
    return mat4_identity();
}

// @brief obtains the world matrix of the given transform by examining its parent (if there is one)
// and multiplying it against the local matrix
// @param t a pointer to the transform whose world matrix to retrieve
// @return a copy of the world matrix
mat4 transform_get_world(transform* t) {
    if (t) {
        mat4 l = transform_get_local(t);
        if (t->parent) {
            mat4 p = transform_get_world(t->parent);
            return mat4_mul(l, p);
        }
        return l;
    }
    // if it fails return a default
    return mat4_identity();
}