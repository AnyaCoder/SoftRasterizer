// stc/math/quaternion.cpp
#include "math/quaternion.h"

quat::quat(const vec3f& axis, float angleRad) {
    float halfAngle = angleRad * 0.5f;
    float s = std::sin(halfAngle);
    w = std::cos(halfAngle);
    vec3f norm_axis = axis.normalized();
    x = norm_axis.x * s;
    y = norm_axis.y * s;
    z = norm_axis.z * s;
}

quat quat::identity() {
    return quat(1.0f, 0.0f, 0.0f, 0.0f);
}

quat quat::fromAxisAngle(const vec3f& axis, float angleRad) {
    return quat(axis, angleRad);
}

float quat::invMagnitudeSquared() const {
    float denominator = w * w + x * x + y * y + z * z;
    if (denominator < 1e-6f) {
        denominator = 1e-6f;
    }
    return 1.0f / denominator;
}

/**
 * @brief Creates a quaternion from Euler angles (in degrees).
 * Applies rotations in ZYX order (Roll around Z, Pitch around X, Yaw around Y).
 * This corresponds to intrinsic Z, then intrinsic X, then intrinsic Y rotation.
 * @param eulerAnglesDegrees Euler angles (x=Pitch, y=Yaw, z=Roll) in degrees.
 * @return Quaternion representing the combined rotation.
 */
quat quat::fromEulerAnglesZYX(const vec3f& eulerAnglesDegrees) {
    vec3f eulerRad = eulerAnglesDegrees * Q_DEG2RAD;
    float cy = std::cos(eulerRad.y * 0.5f); // Cos half Yaw
    float sy = std::sin(eulerRad.y * 0.5f); // Sin half Yaw
    float cp = std::cos(eulerRad.x * 0.5f); // Cos half Pitch
    float sp = std::sin(eulerRad.x * 0.5f); // Sin half Pitch
    float cr = std::cos(eulerRad.z * 0.5f); // Cos half Roll
    float sr = std::sin(eulerRad.z * 0.5f); // Sin half Roll

    quat q;
    q.w = cr * cp * cy + sr * sp * sy;
    q.x = cr * sp * cy + sr * cp * sy; // Pitch rotation component
    q.y = cr * cp * sy - sr * sp * cy; // Yaw rotation component
    q.z = sr * cp * cy - cr * sp * sy; // Roll rotation component
    // Note: The derivation depends heavily on the chosen convention (order and intrinsic/extrinsic).
    // This formula corresponds to ZYX intrinsic. Double-check if a different convention is needed.

    // Alternative using sequential multiplication (more intuitive, maybe slightly slower):
    // Quaternion rollQ(vec3f(0,0,1), eulerRad.z);
    // Quaternion pitchQ(vec3f(1,0,0), eulerRad.x);
    // Quaternion yawQ(vec3f(0,1,0), eulerRad.y);
    // return yawQ * pitchQ * rollQ; // Apply Roll, then Pitch, then Yaw

    return q; // Already normalized if inputs are angles
}

void quat::normalize() {
    float invMag = std::sqrt(invMagnitudeSquared());
    w *= invMag;
    x *= invMag;
    y *= invMag;
    z *= invMag;
}

quat quat::normalized() const {
    quat q = *this;
    q.normalize();
    return q;
}

quat quat::conjugate() const {
    return quat(w, -x, -y, -z);
}

quat quat::inverse() const {
    return conjugate() * invMagnitudeSquared();
}

quat quat::operator+(const quat& other) const {
    return quat(
        w + other.w,
        x + other.x,
        y + other.y,
        z + other.z
    );
}

quat quat::operator-() const {
    return quat(-w, -x, -y, -z);
}

quat quat::operator-(const quat& other) const {
    return quat(
        w - other.w,
        x - other.x,
        y - other.y,
        z - other.z
    );
}


quat quat::operator*(const quat& other) const {
    return quat(
        w * other.w - x * other.x - y * other.y - z * other.z, // New w
        w * other.x + x * other.w + y * other.z - z * other.y, // New x
        w * other.y - x * other.z + y * other.w + z * other.x, // New y
        w * other.z + x * other.y - y * other.x + z * other.w  // New z
    );
}

vec3f quat::operator*(const vec3f& v) const {
    quat p(0.0f, v.x, v.y, v.z);
    quat result = (*this) * p * this->inverse();
    return vec3f(result.x, result.y, result.z);
}

mat3 quat::toMatrix() const {
    return mat3::fromQuaternion(*this);
}

void quat::toAxisAngle(vec3f& axis, float& angleRad) const {
    float clamped_w = std::max(-1.0f, std::min(1.0f, w));
    angleRad = 2.0f * std::acos(clamped_w);
    float s = std::sqrt(1.0f - clamped_w * clamped_w);
    if (s < 1e-4f) { // Avoid division by zero if angle is close to 0 or pi
        // If angle is close to 0, axis doesn't matter much
        // If angle is close to pi, direction can be derived but might be unstable
        axis = vec3f(x, y, z); // Use raw vector part
        if(axis.length() < 1e-6f) axis = vec3f(0.0f, 0.0f, 1.0f); // Default if zero vector
        else axis = axis.normalized();

    } else {
        axis = vec3f(x / s, y / s, z / s);
    }
}

/**
 * @brief Converts this quaternion to Euler angles (in degrees).
 * Returns angles in ZYX order (Roll around Z, Pitch around X, Yaw around Y).
 * Note: Euler angle representation can be ambiguous (gimbal lock).
 * @return vec3f containing Euler angles (x=Pitch, y=Yaw, z=Roll) in degrees.
 */
vec3f quat::toEulerAnglesZYX() const {
    vec3f anglesRad;

    // Ensure quaternion is normalized for accurate conversion
    quat qn = this->normalized();

    // Roll (z-axis rotation)
    float sinr_cosp = 2.0f * (qn.w * qn.z + qn.x * qn.y);
    float cosr_cosp = 1.0f - 2.0f * (qn.y * qn.y + qn.z * qn.z);
    anglesRad.z = std::atan2f(sinr_cosp, cosr_cosp);

    // Pitch (x-axis rotation)
    float sinp = 2.0f * (qn.w * qn.x - qn.y * qn.z);
    if (std::abs(sinp) >= 1.0f) // Use 90 degrees if out of range
        anglesRad.x = std::copysignf(M_PI / 2.0f, sinp);
    else
        anglesRad.x = std::asinf(sinp);

    // Yaw (y-axis rotation)
    float siny_cosp = 2.0f * (qn.w * qn.y + qn.z * qn.x);
    float cosy_cosp = 1.0f - 2.0f * (qn.x * qn.x + qn.y * qn.y);
    anglesRad.y = std::atan2f(siny_cosp, cosy_cosp);

    return anglesRad * Q_RAD2DEG; // Convert radians to degrees
}

// Spherical Linear Interpolation (SLERP) - static method
quat quat::slerp(const quat& q1, const quat& q2, float t) {
    quat q1_norm = q1.normalized();
    quat q2_norm = q2.normalized();

    float dot = q1_norm.w * q2_norm.w + q1_norm.x * q2_norm.x + q1_norm.y * q2_norm.y + q1_norm.z * q2_norm.z;

    // If the dot product is negative, invert one quat to take the shorter path
    quat q2_actual = q2_norm;
    if (dot < 0.0f) {
        q2_actual = -q2_norm;
        dot = -dot;
    }

    // Clamp dot product to prevent domain errors
    dot = std::max(0.0f, std::min(1.0f, dot));

    float theta_0 = std::acos(dot); // angle between rotations

    // Handle case where quats are very close
    if (theta_0 < 1e-6f) {
        return q1_norm; // Or q2_actual, they are almost identical
    }

    float theta = theta_0 * t;      // angle for interpolation
    quat q3 = q2_actual - q1_norm * dot;
    q3.normalize(); // Ensure orthogonality

    return q1_norm * std::cos(theta) + q3 * std::sin(theta);
}