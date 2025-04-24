// src/core/camera.cpp
#include "core/camera.h"
#include <algorithm> // For std::clamp


Camera::Camera(const vec3f& position, float initialYaw, float initialPitch)
    : m_yaw(initialYaw), m_pitch(initialPitch)
{
    m_transform.position = position;
    m_projMatrix = mat4::identity();
    updateCameraVectors();
}

void Camera::setPerspective(float fovDegrees, float aspectRatio, float near, float far) {
    m_projMatrix = mat4::perspective(
        fovDegrees * Q_DEG2RAD,
        aspectRatio,
        near,
        far
    );
}

mat4 Camera::getMVP(const mat4& modelMatrix) const {
    return m_projMatrix * m_viewMatrix * modelMatrix;
}

void Camera::setPosition(const vec3f& position) {
    m_transform.position = position;
    updateCameraVectors();
}

void Camera::setPitchYaw(float pitch, float yaw) {
    m_pitch = pitch;
    m_yaw = yaw;
}

void Camera::setTransform(const Transform& transform) {
    m_transform = transform;
    updateCameraVectors();
}


vec3f Camera::getForward() const {
    return m_transform.rotation * vec3f{0.0f, 0.0f, -1.0f}; // -Z
}

vec3f Camera::getRight() const {
    return m_transform.rotation * vec3f{1.0f, 0.0f, 0.0f}; // +X
}

vec3f Camera::getUp() const {
    return m_transform.rotation * vec3f{0.0f, 1.0f, 0.0f}; // +Y
}

void Camera::updateRotationAndVectors() {
    quat yawQuat = quat::fromAxisAngle(m_worldUp, m_yaw * Q_DEG2RAD);
    vec3f localRight = vec3f{1.0f, 0.0f, 0.0f}; // 固定世界X轴
    quat pitchQuat = quat::fromAxisAngle(localRight, m_pitch * Q_DEG2RAD);
    m_transform.rotation = yawQuat * pitchQuat;
    m_transform.rotation.normalize();
}

void Camera::updateViewMatrix() {
    // Calculates view matrix based on current m_transform.position and m_transform.rotation
    vec3f position = m_transform.position;
    vec3f forward = getForward(); // Uses the current rotation

    // Use the stable lookAt construction method
    vec3f targetPoint = position + forward;
    vec3f actualForward = (targetPoint - position).normalized();
    vec3f actualRight = actualForward.cross(m_worldUp).normalized();
    // Handle potential gimbal lock / numerical instability when looking straight up/down
    if (actualRight.lengthSq() < 1e-6f) {
        quat yawQuatOnly = quat::fromAxisAngle(m_worldUp, m_yaw * Q_DEG2RAD);
        actualRight = yawQuatOnly * vec3f{1.0f, 0.0f, 0.0f}; // Use yaw only for right
    }
    vec3f actualUp = actualRight.cross(actualForward).normalized();

    mat4 rotation = mat4::identity();
    rotation.m[0][0] = actualRight.x; rotation.m[0][1] = actualRight.y; rotation.m[0][2] = actualRight.z;
    rotation.m[1][0] = actualUp.x;    rotation.m[1][1] = actualUp.y;    rotation.m[1][2] = actualUp.z;
    rotation.m[2][0] = -actualForward.x;rotation.m[2][1] = -actualForward.y;rotation.m[2][2] = -actualForward.z;

    mat4 translation = mat4::translation(-position.x, -position.y, -position.z);
    m_viewMatrix = rotation * translation;
}

// Replace the original updateCameraVectors
void Camera::updateCameraVectors() {
    // This function is now less central, maybe only called initially
    updateRotationAndVectors();
    updateViewMatrix();
}

void Camera::processMouseMovement(float xoffset, float yoffset, float sensitivity, bool constrainPitch) {
    xoffset *= sensitivity;
    yoffset *= sensitivity;
    m_yaw += xoffset;
    m_pitch += yoffset;
    m_yaw = fmod(m_yaw, 360.0f);
    if (m_yaw < 0.0f) m_yaw += 360.0f;
    if (constrainPitch) {
        m_pitch = std::clamp(m_pitch, -89.0f, 89.0f);
    }
    updateRotationAndVectors();
    updateViewMatrix();
}

void Camera::processKeyboardMovement(const vec3f& direction, float deltaTime, float speed) {
    float velocity = speed * deltaTime;
    vec3f moveAmount = {0.0f, 0.0f, 0.0f};
    vec3f currentForward = getForward();
    vec3f horizontalRight = -m_worldUp.cross(currentForward).normalized();
    // Forward/Backward (uses camera's forward)
    moveAmount = moveAmount + currentForward * direction.z * velocity;
    // Right/Left (Strafe - uses camera's right)
    moveAmount = moveAmount + horizontalRight * direction.x * velocity;
    // Up/Down (uses World Up vector)
    moveAmount = moveAmount + m_worldUp * direction.y * velocity;

    m_transform.position = m_transform.position + moveAmount;

    updateViewMatrix(); 

}