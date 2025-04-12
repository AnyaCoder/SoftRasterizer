// src/math/matrix.h
#pragma once
#include "math/matrix.h"
#include "math/vector.h"

mat4::mat4() {
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            m[i][j] = 0.0f;
}

mat4::mat4(float v) {
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            m[i][j] = (i == j ? v : 0.0f);
}

mat4 mat4::identity() {
    return mat4(1.0f);
}

mat4 mat4::translation(float x, float y, float z) {
    mat4 mat(1.0f);
    mat.m[0][3] = x;
    mat.m[1][3] = y;
    mat.m[2][3] = z;
    return mat;
}

mat4 mat4::scale(float x, float y, float z) {
    mat4 mat(1.0f);
    mat.m[0][0] = x;
    mat.m[1][1] = y;
    mat.m[2][2] = z;
    return mat;
}

mat4 mat4::rotationX(float angle) {
    mat4 mat(1.0f);
    float c = std::cos(angle);
    float s = std::sin(angle);
    mat.m[1][1] = c;
    mat.m[1][2] = -s;
    mat.m[2][1] = s;
    mat.m[2][2] = c;
    return mat;
}

mat4 mat4::rotationY(float angle) {
    mat4 mat(1.0f);
    float c = std::cos(angle);
    float s = std::sin(angle);
    mat.m[0][0] = c;
    mat.m[0][2] = s;
    mat.m[2][0] = -s;
    mat.m[2][2] = c;
    return mat;
}

mat4 mat4::rotationZ(float angle) {
    mat4 mat(1.0f);
    float c = std::cos(angle);
    float s = std::sin(angle);
    mat.m[0][0] = c;
    mat.m[0][1] = -s;
    mat.m[1][0] = s;
    mat.m[1][1] = c;
    return mat;
}

mat4 mat4::perspective(float fov, float aspect, float near, float far) {
    // Ensure input validity to avoid division by zero or degenerate cases
    if (aspect <= 0 || far <= near || near <= 0 || fov <= 0 || fov >= 3.1415926f) {
        return mat4::identity();
    }
    mat4 mat;
    float tanHalfFov = std::tan(fov / 2.0f);
    mat.m[0][0] = 1.0f / (aspect * tanHalfFov);
    mat.m[1][1] = 1.0f / tanHalfFov;
    mat.m[2][2] = -(far + near) / (far - near);
    mat.m[2][3] = -2.0f * far * near / (far - near);
    mat.m[3][2] = -1.0f;
    mat.m[3][3] = 0.0f;
    return mat;
}

mat4& mat4::operator=(const mat4& other) {
    if (this == &other) { // Handle self-assignment
        return *this;
    }
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            m[i][j] = other.m[i][j];
        }
    }
    return *this;
}

mat4 mat4::operator*(const mat4& other) const {
    mat4 result;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            result.m[i][j] = 0.0f;
            for (int k = 0; k < 4; k++) {
                result.m[i][j] += m[i][k] * other.m[k][j];
            }
        }
    }
    return result;
}

vec4f mat4::operator*(const vec4f& v) const {
    vec4f result;
    result.x = m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z + m[0][3] * v.w;
    result.y = m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z + m[1][3] * v.w;
    result.z = m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z + m[2][3] * v.w;
    result.w = m[3][0] * v.x + m[3][1] * v.y + m[3][2] * v.z + m[3][3] * v.w;
    return result;
}

mat4 mat4::transpose() const {
    mat4 result;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            result.m[j][i] = m[i][j]; // Swap row and column indices
        }
    }
    return result;
}

mat4 mat4::inverse() const {
    // Implementation based on common optimized methods (e.g., derived from MESA GLU __gluInvertMatrixd)
    // Calculates the adjugate matrix elements directly.
    mat4 inv; // Stores the adjugate first, then the inverse.

    inv.m[0][0] = m[1][1] * m[2][2] * m[3][3] - m[1][1] * m[2][3] * m[3][2] - m[2][1] * m[1][2] * m[3][3] + m[2][1] * m[1][3] * m[3][2] + m[3][1] * m[1][2] * m[2][3] - m[3][1] * m[1][3] * m[2][2];
    inv.m[1][0] = -m[1][0] * m[2][2] * m[3][3] + m[1][0] * m[2][3] * m[3][2] + m[2][0] * m[1][2] * m[3][3] - m[2][0] * m[1][3] * m[3][2] - m[3][0] * m[1][2] * m[2][3] + m[3][0] * m[1][3] * m[2][2];
    inv.m[2][0] = m[1][0] * m[2][1] * m[3][3] - m[1][0] * m[2][3] * m[3][1] - m[2][0] * m[1][1] * m[3][3] + m[2][0] * m[1][3] * m[3][1] + m[3][0] * m[1][1] * m[2][3] - m[3][0] * m[1][3] * m[2][1];
    inv.m[3][0] = -m[1][0] * m[2][1] * m[3][2] + m[1][0] * m[2][2] * m[3][1] + m[2][0] * m[1][1] * m[3][2] - m[2][0] * m[1][2] * m[3][1] - m[3][0] * m[1][1] * m[2][2] + m[3][0] * m[1][2] * m[2][1];

    inv.m[0][1] = -m[0][1] * m[2][2] * m[3][3] + m[0][1] * m[2][3] * m[3][2] + m[2][1] * m[0][2] * m[3][3] - m[2][1] * m[0][3] * m[3][2] - m[3][1] * m[0][2] * m[2][3] + m[3][1] * m[0][3] * m[2][2];
    inv.m[1][1] = m[0][0] * m[2][2] * m[3][3] - m[0][0] * m[2][3] * m[3][2] - m[2][0] * m[0][2] * m[3][3] + m[2][0] * m[0][3] * m[3][2] + m[3][0] * m[0][2] * m[2][3] - m[3][0] * m[0][3] * m[2][2];
    inv.m[2][1] = -m[0][0] * m[2][1] * m[3][3] + m[0][0] * m[2][3] * m[3][1] + m[2][0] * m[0][1] * m[3][3] - m[2][0] * m[0][3] * m[3][1] - m[3][0] * m[0][1] * m[2][3] + m[3][0] * m[0][3] * m[2][1];
    inv.m[3][1] = m[0][0] * m[2][1] * m[3][2] - m[0][0] * m[2][2] * m[3][1] - m[2][0] * m[0][1] * m[3][2] + m[2][0] * m[0][2] * m[3][1] + m[3][0] * m[0][1] * m[2][2] - m[3][0] * m[0][2] * m[2][1];

    inv.m[0][2] = m[0][1] * m[1][2] * m[3][3] - m[0][1] * m[1][3] * m[3][2] - m[1][1] * m[0][2] * m[3][3] + m[1][1] * m[0][3] * m[3][2] + m[3][1] * m[0][2] * m[1][3] - m[3][1] * m[0][3] * m[1][2];
    inv.m[1][2] = -m[0][0] * m[1][2] * m[3][3] + m[0][0] * m[1][3] * m[3][2] + m[1][0] * m[0][2] * m[3][3] - m[1][0] * m[0][3] * m[3][2] - m[3][0] * m[0][2] * m[1][3] + m[3][0] * m[0][3] * m[1][2];
    inv.m[2][2] = m[0][0] * m[1][1] * m[3][3] - m[0][0] * m[1][3] * m[3][1] - m[1][0] * m[0][1] * m[3][3] + m[1][0] * m[0][3] * m[3][1] + m[3][0] * m[0][1] * m[1][3] - m[3][0] * m[0][3] * m[1][1];
    inv.m[3][2] = -m[0][0] * m[1][1] * m[3][2] + m[0][0] * m[1][2] * m[3][1] + m[1][0] * m[0][1] * m[3][2] - m[1][0] * m[0][2] * m[3][1] - m[3][0] * m[0][1] * m[1][2] + m[3][0] * m[0][2] * m[1][1];

    inv.m[0][3] = -m[0][1] * m[1][2] * m[2][3] + m[0][1] * m[1][3] * m[2][2] + m[1][1] * m[0][2] * m[2][3] - m[1][1] * m[0][3] * m[2][2] - m[2][1] * m[0][2] * m[1][3] + m[2][1] * m[0][3] * m[1][2];
    inv.m[1][3] = m[0][0] * m[1][2] * m[2][3] - m[0][0] * m[1][3] * m[2][2] - m[1][0] * m[0][2] * m[2][3] + m[1][0] * m[0][3] * m[2][2] + m[2][0] * m[0][2] * m[1][3] - m[2][0] * m[0][3] * m[1][2];
    inv.m[2][3] = -m[0][0] * m[1][1] * m[2][3] + m[0][0] * m[1][3] * m[2][1] + m[1][0] * m[0][1] * m[2][3] - m[1][0] * m[0][3] * m[2][1] - m[2][0] * m[0][1] * m[1][3] + m[2][0] * m[0][3] * m[1][1];
    inv.m[3][3] = m[0][0] * m[1][1] * m[2][2] - m[0][0] * m[1][2] * m[2][1] - m[1][0] * m[0][1] * m[2][2] + m[1][0] * m[0][2] * m[2][1] + m[2][0] * m[0][1] * m[1][2] - m[2][0] * m[0][2] * m[1][1];

    // Calculate determinant using the first column and the calculated cofactors (stored in the first row of 'inv' before division)
    float det = m[0][0] * inv.m[0][0] + m[1][0] * inv.m[0][1] + m[2][0] * inv.m[0][2] + m[3][0] * inv.m[0][3];

    // Check for singularity (determinant close to zero)
    // Use a small epsilon comparison for floating-point numbers
    const float epsilon = 1e-6f; // Adjust epsilon based on required precision
    if (std::fabs(det) < epsilon) {
            // Matrix is singular or close to singular. Cannot invert reliably.
            // Return identity matrix as a fallback.
            // Consider logging a warning or throwing an exception in a real application.
            std::cerr << "Warning: Matrix determinant is close to zero (" << det << "). Returning identity matrix." << std::endl;
            return mat4::identity();
    }

    // Divide the adjugate matrix by the determinant to get the inverse
    float invDet = 1.0f / det;

    mat4 result; // Final inverse matrix
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            result.m[i][j] = inv.m[i][j] * invDet;
        }
    }

    return result;
}

