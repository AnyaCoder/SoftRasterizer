// include/math/matrix.h
#pragma once
#include "vector.h"
#include <cmath>
#include <iostream>

struct mat4 {
    float m[4][4];

    mat4();

    mat4(float v);

    static mat4 identity();

    static mat4 translation(float x, float y, float z);

    static mat4 scale(float x, float y, float z);

    static mat4 rotationX(float angle);
    static mat4 rotationY(float angle);
    static mat4 rotationZ(float angle);

    static mat4 perspective(float fov, float aspect, float near, float far);

    mat4& operator=(const mat4& other);
    mat4 operator*(const mat4& other) const;
    vec4f operator*(const vec4f& v) const;

    mat4 transpose() const;
    mat4 inverse() const;
};
