// include/math/transform.h
#pragma once

#include "math/vector.h"
#include "math/quaternion.h"
#include "math/matrix.h"

class Transform {
public:
    vec3f position;
    quat rotation;
    vec3f scale;

    // --- Constructors ---
    Transform();
    Transform(const vec3f& pos, const quat& rot, const vec3f& scl);
    Transform(const vec3f& pos, const vec3f& eulerAnglesDegreesZYX, const vec3f& scl);

    // --- Setters ---
    void setPosition(const vec3f& pos);
    void setRotation(const quat& rot);
    void setRotationEulerZYX(const vec3f& eulerAnglesDegreesZYX);
    void setScale(const vec3f& scl);

    // --- Getters ---
    const vec3f& getPosition() const { return position; }
    const quat& getRotation() const { return rotation; }
    const vec3f& getScale() const { return scale; }
    vec3f getRotationEulerZYX() const;

    // --- Transformations ---
    void translate(const vec3f& delta);
    void translateLocal(const vec3f& delta);
    void rotate(const quat& delta);
    void rotateAround(const vec3f& point, const vec3f& axis, float angleRad);
    void rotateEulerZYX(const vec3f& deltaEulerDegreesZYX);

    // --- Matrix Generation ---
    mat4 getTransformMatrix() const;
    mat3 getNormalMatrix() const;

    // --- Hierarchy ---
    Transform combine(const Transform& parent) const;
    void lookAt(const vec3f& target, const vec3f& worldUp = vec3f(0.0f, 1.0f, 0.0f));
};