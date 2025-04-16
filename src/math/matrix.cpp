// src/math/matrix.cpp
#pragma once
#include "math/matrix.h"
#include "math/vector.h"


mat3::mat3() {
    memset(&m, 0, sizeof(m));
}

mat3::mat3(float v) {
    memset(&m, 0, sizeof(m));
    for (int i = 0; i < 3; i++) m[i][i] = v;
}

mat3 mat3::identity() {
    return mat3(1.0f);
}

mat4 mat3::toMat4() {
    mat4 newMat;
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            newMat.m[i][j] = m[i][j];
    newMat.m[3][3] = 1.0f;
    return newMat;
}

mat3 mat3::scale(float x, float y, float z) {
    mat3 mat(1.0f);
    mat.m[0][0] = x;
    mat.m[1][1] = y;
    mat.m[2][2] = z;
    return mat;
}

mat3 mat3::fromQuaternion(const quat& q) {
    mat3 result = mat3::identity();
    quat qn = q.normalized();

    float xx = qn.x * qn.x; float xy = qn.x * qn.y; float xz = qn.x * qn.z; float xw = qn.x * qn.w;
    float yy = qn.y * qn.y; float yz = qn.y * qn.z; float yw = qn.y * qn.w;
    float zz = qn.z * qn.z; float zw = qn.z * qn.w;

    result.m[0][0] = 1.0f - 2.0f * (yy + zz);
    result.m[0][1] = 2.0f * (xy - zw);
    result.m[0][2] = 2.0f * (xz + yw);

    result.m[1][0] = 2.0f * (xy + zw);
    result.m[1][1] = 1.0f - 2.0f * (xx + zz);
    result.m[1][2] = 2.0f * (yz - xw);

    result.m[2][0] = 2.0f * (xz - yw);
    result.m[2][1] = 2.0f * (yz + xw);
    result.m[2][2] = 1.0f - 2.0f * (xx + yy);

    return result;
}

quat mat3::toQuat() const {
    // Algorithm based on Ken Shoemake's "Quaternion Calculus and Fast Animation"
    quat q;

    float trace = m[0][0] + m[1][1] + m[2][2];

    if (trace > 0.0f) {
        // Calculate S = 4 * qw
        float S = std::sqrt(trace + 1.0f) * 2.0f;
        q.w = 0.25f * S;
        // Calculate remaining components using off-diagonal elements
        q.x = (m[2][1] - m[1][2]) / S;
        q.y = (m[0][2] - m[2][0]) / S;
        q.z = (m[1][0] - m[0][1]) / S;
    } else if ((m[0][0] > m[1][1]) && (m[0][0] > m[2][2])) {
        // Column 0 has largest diagonal element
        // Calculate S = 4 * qx
        float S = std::sqrt(1.0f + m[0][0] - m[1][1] - m[2][2]) * 2.0f;
        q.w = (m[2][1] - m[1][2]) / S;
        q.x = 0.25f * S;
        q.y = (m[0][1] + m[1][0]) / S;
        q.z = (m[0][2] + m[2][0]) / S;
    } else if (m[1][1] > m[2][2]) {
        // Column 1 has largest diagonal element
        // Calculate S = 4 * qy
        float S = std::sqrt(1.0f + m[1][1] - m[0][0] - m[2][2]) * 2.0f;
        q.w = (m[0][2] - m[2][0]) / S;
        q.x = (m[0][1] + m[1][0]) / S;
        q.y = 0.25f * S;
        q.z = (m[1][2] + m[2][1]) / S;
    } else {
        // Column 2 has largest diagonal element
        // Calculate S = 4 * qz
        float S = std::sqrt(1.0f + m[2][2] - m[0][0] - m[1][1]) * 2.0f;
        q.w = (m[1][0] - m[0][1]) / S;
        q.x = (m[0][2] + m[2][0]) / S;
        q.y = (m[1][2] + m[2][1]) / S;
        q.z = 0.25f * S;
    }

    return q;
}

mat3& mat3::operator=(const mat3& other) {
    if (this != &other) {
        memcpy(m, other.m, sizeof(float) * 3 * 3);
    }
    return *this;
}

mat3 mat3::operator*(const mat3& other) const {
#ifdef NaiveMethod
    mat3 result;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            for (int k = 0; k < 3; k++) {
                result.m[i][j] += m[i][k] * other.m[k][j];
            }
        }
    }
    return result;
#else
    return multiply_3x3_unrolled(*this, other);
#endif
}

/**
 * @brief Multiplies two 3x3 matrices (a * b) using full loop unrolling.
 * @param a Left-hand side matrix.
 * @param b Right-hand side matrix.
 * @return The resulting matrix product.
 */
mat3 mat3::multiply_3x3_unrolled(const mat3& a, const mat3& b) {
    mat3 result; // Result matrix C = A * B

    // Row 0
    result.m[0][0] = a.m[0][0] * b.m[0][0] + a.m[0][1] * b.m[1][0] + a.m[0][2] * b.m[2][0];
    result.m[0][1] = a.m[0][0] * b.m[0][1] + a.m[0][1] * b.m[1][1] + a.m[0][2] * b.m[2][1];
    result.m[0][2] = a.m[0][0] * b.m[0][2] + a.m[0][1] * b.m[1][2] + a.m[0][2] * b.m[2][2];

    // Row 1
    result.m[1][0] = a.m[1][0] * b.m[0][0] + a.m[1][1] * b.m[1][0] + a.m[1][2] * b.m[2][0];
    result.m[1][1] = a.m[1][0] * b.m[0][1] + a.m[1][1] * b.m[1][1] + a.m[1][2] * b.m[2][1];
    result.m[1][2] = a.m[1][0] * b.m[0][2] + a.m[1][1] * b.m[1][2] + a.m[1][2] * b.m[2][2];

    // Row 2
    result.m[2][0] = a.m[2][0] * b.m[0][0] + a.m[2][1] * b.m[1][0] + a.m[2][2] * b.m[2][0];
    result.m[2][1] = a.m[2][0] * b.m[0][1] + a.m[2][1] * b.m[1][1] + a.m[2][2] * b.m[2][1];
    result.m[2][2] = a.m[2][0] * b.m[0][2] + a.m[2][1] * b.m[1][2] + a.m[2][2] * b.m[2][2];

    return result;
}

vec3f mat3::operator*(const vec3f& v) const {
    vec3f result;
    result.x = m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z;
    result.y = m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z;
    result.z = m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z;
    return result;
}

mat3 mat3::transpose() const {
    mat3 result;
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            result.m[j][i] = m[i][j];
        }
    }
    return result;
}

mat3 mat3::inverse() const {
    // Calculates the adjugate matrix elements directly.
    mat3 inv; // Stores the adjugate first, then the inverse.

    // Calculate cofactors and transpose directly into 'inv' to form the adjugate
    inv.m[0][0] = m[1][1] * m[2][2] - m[1][2] * m[2][1];
    inv.m[1][0] = m[1][2] * m[2][0] - m[1][0] * m[2][2];
    inv.m[2][0] = m[1][0] * m[2][1] - m[1][1] * m[2][0];

    inv.m[0][1] = m[0][2] * m[2][1] - m[0][1] * m[2][2];
    inv.m[1][1] = m[0][0] * m[2][2] - m[0][2] * m[2][0];
    inv.m[2][1] = m[0][1] * m[2][0] - m[0][0] * m[2][1];

    inv.m[0][2] = m[0][1] * m[1][2] - m[0][2] * m[1][1];
    inv.m[1][2] = m[0][2] * m[1][0] - m[0][0] * m[1][2];
    inv.m[2][2] = m[0][0] * m[1][1] - m[0][1] * m[1][0];


    // Calculate determinant using the first column of the original matrix
    float det = m[0][0] * inv.m[0][0] + m[1][0] * inv.m[0][1] + m[2][0] * inv.m[0][2];

    // Check for singularity (determinant close to zero)
    const float epsilon = 1e-6f; // Adjust epsilon based on required precision
    if (std::fabs(det) < epsilon) {
        std::cerr << "Warning: Matrix3x3 determinant is close to zero (" << det << "). Returning identity matrix." << std::endl;
        return mat3::identity();
    }

    // Divide the adjugate matrix by the determinant to get the inverse
    float invDet = 1.0f / det;

    mat3 result; // Final inverse matrix
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            result.m[i][j] = inv.m[i][j] * invDet;
        }
    }

    return result;
}


/************************************* */
mat4::mat4() {
    memset(&m, 0, sizeof(m));
}

mat4::mat4(float v) {
    memset(&m, 0, sizeof(m));
    for (int i = 0; i < 4; i++) m[i][i] = v;
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
    return mat3::scale(x, y, z).toMat4();
}

mat4 mat4::rotationX(float angleRad) {
    mat4 mat(1.0f);
    float c = std::cos(angleRad);
    float s = std::sin(angleRad);
    mat.m[1][1] = c;
    mat.m[1][2] = -s;
    mat.m[2][1] = s;
    mat.m[2][2] = c;
    return mat;
}

mat4 mat4::rotationY(float angleRad) {
    mat4 mat(1.0f);
    float c = std::cos(angleRad);
    float s = std::sin(angleRad);
    mat.m[0][0] = c;
    mat.m[0][2] = s;
    mat.m[2][0] = -s;
    mat.m[2][2] = c;
    return mat;
}

mat4 mat4::rotationZ(float angleRad) {
    mat4 mat(1.0f);
    float c = std::cos(angleRad);
    float s = std::sin(angleRad);
    mat.m[0][0] = c;
    mat.m[0][1] = -s;
    mat.m[1][0] = s;
    mat.m[1][1] = c;
    return mat;
}

mat4 mat4::perspective(float fovRad, float aspect, float near, float far) {
    // Ensure input validity to avoid division by zero or degenerate cases
    if (aspect <= 0 || far <= near || near <= 0 || fovRad <= 0 || fovRad >= 3.1415926f) {
        return mat4::identity();
    }
    mat4 mat;
    float tanHalfFov = std::tan(fovRad / 2.0f);
    mat.m[0][0] = 1.0f / (aspect * tanHalfFov);
    mat.m[1][1] = 1.0f / tanHalfFov;
    mat.m[2][2] = -(far + near) / (far - near);
    mat.m[2][3] = -2.0f * far * near / (far - near);
    mat.m[3][2] = -1.0f;
    mat.m[3][3] = 0.0f;
    return mat;
}

mat4 mat4::fromQuaternion(const quat& q) {
    return mat3::fromQuaternion(q).toMat4();
}

mat4& mat4::operator=(const mat4& other) {
#ifdef NaiveMethod
    if (this == &other) { // Handle self-assignment
        return *this;
    }
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            m[i][j] = other.m[i][j];
        }
    }
    return *this;
#else
    if (this != &other) {
        _mm_store_ps(&m[0][0], _mm_load_ps(&other.m[0][0]));
        _mm_store_ps(&m[1][0], _mm_load_ps(&other.m[1][0]));
        _mm_store_ps(&m[2][0], _mm_load_ps(&other.m[2][0]));
        _mm_store_ps(&m[3][0], _mm_load_ps(&other.m[3][0]));
    }
    return *this;
#endif
}

mat4 mat4::operator*(const mat4& other) const {
#ifdef NaiveMethod
    mat4 result;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            for (int k = 0; k < 4; k++) {
                result.m[i][j] += m[i][k] * other.m[k][j];
            }
        }
    }
    return result;
#else
    return multiply_4x4_sse(*this, other);
#endif
}

/**
 * @brief Multiplies two 4x4 matrices (a * b) using SSE intrinsics.
 * @details This version can be significantly faster on CPUs supporting SSE/SSE2.
 * Assumes row-major storage for matrices.
 * @param a Left-hand side matrix.
 * @param b Right-hand side matrix.
 * @return The resulting matrix product.
 */
mat4 mat4::multiply_4x4_sse(const mat4& a, const mat4& b) {
    mat4 result;
    // Treat matrix rows/columns as __m128 (4 floats)
    // We calculate one row of the result matrix C at a time.
    // C[i] = A[i][0]*B[0] + A[i][1]*B[1] + A[i][2]*B[2] + A[i][3]*B[3]
    // Where C[i], B[0..3] are rows treated as __m128 vectors.

    // Pointer casting for easier __m128 loading (use with care, depends on structure layout)
    const float* pA = &a.m[0][0];
    const float* pB = &b.m[0][0];
    float* pResult = &result.m[0][0];

    // Load rows of B
    __m128 b_row0 = _mm_loadu_ps(&pB[0 * 4]); // B[0]
    __m128 b_row1 = _mm_loadu_ps(&pB[1 * 4]); // B[1]
    __m128 b_row2 = _mm_loadu_ps(&pB[2 * 4]); // B[2]
    __m128 b_row3 = _mm_loadu_ps(&pB[3 * 4]); // B[3]

    for (int i = 0; i < 4; ++i) {
        // Get current row i of A
        const float* a_row_ptr = &pA[i * 4];

        // Broadcast elements of A's row i and multiply/add
        // C[i] = A[i][0]*B[0]
        __m128 result_row = _mm_mul_ps(_mm_set1_ps(a_row_ptr[0]), b_row0);
        // C[i] += A[i][1]*B[1]
        result_row = _mm_add_ps(result_row, _mm_mul_ps(_mm_set1_ps(a_row_ptr[1]), b_row1));
        // C[i] += A[i][2]*B[2]
        result_row = _mm_add_ps(result_row, _mm_mul_ps(_mm_set1_ps(a_row_ptr[2]), b_row2));
        // C[i] += A[i][3]*B[3]
        result_row = _mm_add_ps(result_row, _mm_mul_ps(_mm_set1_ps(a_row_ptr[3]), b_row3));

        // Store the resulting row i into the result matrix
        _mm_storeu_ps(&pResult[i * 4], result_row);
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

