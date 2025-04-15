// include/math/quaternion.h
#pragma once

#include "math/vector.h"
#include "math/matrix.h"
#include <cmath>
#include <vector>

constexpr float M_PI = 3.14159265f;
constexpr float Q_DEG2RAD = M_PI / 180.0f;
constexpr float Q_RAD2DEG = 180.0f / M_PI;

struct mat3;

struct quat {
public:
    float w, x, y, z;
    quat() : w(1.0f), x(0.0f), y(0.0f), z(0.0f) {}
    quat(float w, float x, float y, float z) : w(w), x(x), y(y), z(z) {}
    quat(const vec3f& axis, float angleRad);

    static quat identity();
    static quat fromAxisAngle(const vec3f& axis, float angleRad);
    float invMagnitudeSquared() const;
    static quat fromEulerAnglesZYX(const vec3f& eulerAnglesDegrees);

    // --- Methods ---
    void normalize();
    quat normalized() const;
    quat conjugate() const;
    quat inverse() const;

    // Operators
    quat operator+(const quat& other) const;
    quat operator-() const;
    quat operator-(const quat& other) const;

    template<typename T>
    quat operator*(T s) const {
        return quat(w * s, x * s, y * s, z * s);
    }
    quat operator*(const quat& other) const;
    vec3f operator*(const vec3f& v) const;

    // Conversions
    mat3 toMatrix() const;
    void toAxisAngle(vec3f& axis, float& angleRad) const;
    vec3f toEulerAnglesZYX() const;

    // Spherical Linear Interpolation (SLERP) - static method
    static quat slerp(const quat& q1, const quat& q2, float t);
};