// src/core/camera.cpp
#include "core/camera.h"

Camera::Camera(const vec3f& position, const vec3f& target, const vec3f& up)
    : m_target(target), m_up(up) {
    m_transform.position = position;
    updateViewMatrix();
}

void Camera::setPerspective(float fovDegrees, float aspectRatio, float near, float far) {
    m_projMatrix = mat4::perspective(
        fovDegrees * 3.1415926f / 180.0f, // 转换为弧度
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
    updateViewMatrix();
}

void Camera::setTarget(const vec3f& target) {
    m_target = target;
    updateViewMatrix();
}

void Camera::setRotation(const vec3f& rotation) {
    m_transform.rotation = quat::fromEulerAnglesZYX(rotation);
    updateViewMatrix();
}

void Camera::setTransform(const Transform& transform) {
    m_transform = transform;
    updateViewMatrix();
}

void Camera::updateViewMatrix() {
    // 计算视图矩阵（lookAt 实现）
    vec3f pos = m_transform.position;
    vec3f forward = (m_target - pos).normalized(); // 朝向向量
    vec3f right = forward.cross(m_up).normalized();       // 右向量
    vec3f up = right.cross(forward).normalized();         // 调整后的上向量

    // 视图矩阵的旋转部分
    mat4 rotation = mat4::identity();
    rotation.m[0][0] = right.x;    rotation.m[0][1] = right.y;    rotation.m[0][2] = right.z;   
    rotation.m[1][0] = up.x;       rotation.m[1][1] = up.y;       rotation.m[1][2] = up.z;      
    rotation.m[2][0] = -forward.x; rotation.m[2][1] = -forward.y; rotation.m[2][2] = -forward.z;

    // 平移部分
    mat4 translation = mat4::translation(-pos.x, -pos.y, -pos.z);

    // 视图矩阵 = 旋转 * 平移
    m_viewMatrix = rotation * translation;
}