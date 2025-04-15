// include/math/matrix.h
#pragma once
#include "math/vector.h"
#include "math/quaternion.h"
#include <cmath>
#include <cstring>
#include <iostream>

#ifndef Naive_matmul
#include <immintrin.h>
// #define Naive_matmul
#endif

struct quat;
struct mat4;

struct mat3 {
    float m[3][3];

    mat3();
    mat3(float v);
    mat4 toMat4();
    static mat3 identity();
    static mat3 scale(float x, float y, float z);
    static mat3 fromQuaternion(const quat& q);
    quat toQuat() const;
    mat3 operator*(const mat3& other) const;
    vec3f operator*(const vec3f& v) const;
    static mat3 multiply_3x3_unrolled(const mat3& a, const mat3& b);

    mat3 transpose() const;
    mat3 inverse() const;
};

struct mat4 {
    float m[4][4];

    mat4();
    mat4(float v);
    static mat4 identity();
    static mat4 translation(float x, float y, float z);
    static mat4 scale(float x, float y, float z);
    static mat4 rotationX(float angleRad);
    static mat4 rotationY(float angleRad);
    static mat4 rotationZ(float angleRad);
    static mat4 perspective(float fovRad, float aspect, float near, float far);
    static mat4 fromQuaternion(const quat& q);

    mat4& operator=(const mat4& other);
    mat4 operator*(const mat4& other) const;
    vec4f operator*(const vec4f& v) const;
    static mat4 multiply_4x4_sse(const mat4& a, const mat4& b);

    mat4 transpose() const;
    mat4 inverse() const;
};
