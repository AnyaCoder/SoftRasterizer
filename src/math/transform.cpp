// src/math/transform.cpp

#include "math/transform.h"

Transform::Transform() : position(0.0f, 0.0f, 0.0f), rotation(), scale(1.0f, 1.0f, 1.0f) {}

Transform::Transform(const vec3f& pos, const quat& rot, const vec3f& scl)
        : position(pos), rotation(rot.normalized()), scale(scl) {}

Transform::Transform(const vec3f& pos, const vec3f& eulerAnglesDegreesZYX, const vec3f& scl)
    : position(pos), scale(scl) {
    rotation = quat::fromEulerAnglesZYX(eulerAnglesDegreesZYX);
}

void Transform::setPosition(const vec3f& pos) { position = pos; }
void Transform::setRotation(const quat& rot) { rotation = rot.normalized(); } // Store normalized
void Transform::setRotationEulerZYX(const vec3f& eulerAnglesDegreesZYX) {
    rotation = quat::fromEulerAnglesZYX(eulerAnglesDegreesZYX);
}
void Transform::setScale(const vec3f& scl) { scale = scl; }

/**
 * @brief Gets the rotation represented as Euler angles (in degrees).
 * Returns angles in ZYX order convention (Yaw, Pitch, Roll).
 * Note: Conversion back to Euler angles can be ambiguous.
 * @return vec3f Euler angles (x=Pitch, y=Yaw, z=Roll) in degrees.
 */
vec3f Transform::getRotationEulerZYX() const {
    return rotation.toEulerAnglesZYX();
}

// --- Transformations ---
// Translate by a vector delta
void Transform::translate(const vec3f& delta) {
    position = position + delta;
}

// Translate relative to current rotation
void Transform::translateLocal(const vec3f& delta) {
    position = position + (rotation * delta);
}

// Rotate by composing with another quat (applies delta rotation AFTER current)
void Transform::rotate(const quat& delta) {
    rotation = (delta.normalized() * rotation).normalized(); // Keep normalized
}

// Rotate around a world-space axis
void Transform::rotateAround(const vec3f& point, const vec3f& axis, float angleRad) {
    quat rotDelta = quat::fromAxisAngle(axis, angleRad);
    vec3f dir = position - point;
    position = point + (rotDelta * dir); // Rotate position vector around point
    rotation = (rotDelta * rotation).normalized(); // Rotate orientation
}

/**
 * @brief Rotates the transform by the specified Euler angles (in degrees) relative to the current rotation.
 * Applies delta rotation AFTER current rotation. Uses ZYX convention for delta.
 * @param deltaEulerDegreesZYX Amount to rotate (x=Pitch, y=Yaw, z=Roll) in degrees.
 */
void Transform::rotateEulerZYX(const vec3f& deltaEulerDegreesZYX) {
    quat deltaRot = quat::fromEulerAnglesZYX(deltaEulerDegreesZYX);
    // Apply delta rotation: new = delta * old
    rotation = (deltaRot * rotation).normalized();
}

// Get the final 4x4 transformation matrix (Scale -> Rotate -> Translate)
mat4 Transform::getTransformMatrix() const {
    mat4 scaleMat = mat4::scale(scale.x, scale.y, scale.z);
    mat3 rotMat = rotation.toMatrix(); // Assumes rotation is normalized
    mat4 transMat = mat4::translation(position.x, position.y, position.z);

    // Combine: Scale first, then Rotate, then Translate
    return transMat * rotMat.toMat4() * scaleMat;
}

mat3 Transform::getNormalMatrix() const {
    mat3 modelMat = rotation.toMatrix(); 
    mat3 scaleMat = mat3::scale(scale.x, scale.y, scale.z);
    mat3 rotScaleMat = modelMat * scaleMat;
    mat3 normalMat = rotScaleMat.inverse().transpose();
    return normalMat;
}

// --- Hierarchy ---
// Combine this local transform with a parent's world transform
Transform Transform::combine(const Transform& parent) const {
    Transform worldTransform;
    // Scale: Component-wise multiplication
    worldTransform.scale = vec3f(parent.scale.x * scale.x, parent.scale.y * scale.y, parent.scale.z * scale.z);
    // Rotation: Compose rotations
    worldTransform.rotation = (parent.rotation * rotation).normalized();
    // Position: Rotate/Scale local position by parent, then add parent position
    worldTransform.position = parent.position + \
        (parent.rotation * vec3f(parent.scale.x * position.x, parent.scale.y * position.y, parent.scale.z * position.z));
    return worldTransform;
}

// --- LookAt ---
// Orients the transform to look at a target point from its current position
void Transform::lookAt(const vec3f& target, const vec3f& worldUp) {
    vec3f forward = (target - position).normalized();
    if (forward.lengthSq() < 1e-6f) return; // Cannot look at self

    vec3f right = worldUp.cross(forward).normalized();
    // Handle case where forward is parallel to worldUp
    if (right.lengthSq() < 1e-6f) {
        // Use a different temporary up vector
        vec3f altUp = (std::abs(worldUp.y) < 0.99f) ? vec3f(0.0f, 1.0f, 0.0f) : vec3f(0.0f, 0.0f, 1.0f);
        if(std::abs(forward.y - altUp.y) < 1e-6f) altUp = vec3f(1.0f, 0.0f, 0.0f); // Another fallback
        right = altUp.cross(forward).normalized();
        if (right.lengthSq() < 1e-6f) right = vec3f(1.0f, 0.0f, 0.0f); // Final fallback
    }

    vec3f up = forward.cross(right).normalized(); // Orthonormal up

    // Create rotation matrix from basis vectors [Right, Up, -Forward]
    mat3 lookMat = mat3::identity();
    lookMat.m[0][0] = right.x; lookMat.m[1][0] = right.y; lookMat.m[2][0] = right.z;
    lookMat.m[0][1] = up.x;    lookMat.m[1][1] = up.y;    lookMat.m[2][1] = up.z;
    lookMat.m[0][2] = -forward.x;lookMat.m[1][2] = -forward.y;lookMat.m[2][2] = -forward.z;
    rotation = lookMat.toQuat().normalized();
}