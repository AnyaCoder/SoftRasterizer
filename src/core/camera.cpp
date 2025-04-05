#include "core/camera.h"

Camera::Camera(const vec3f& position, const vec3f& target, const vec3f& up)
    : m_position(position), m_target(target), m_up(up) {
    updateViewMatrix();
    m_projMatrix = Matrix4x4::identity(); // 初始化为单位矩阵，等待 setPerspective 设置
}

void Camera::setPerspective(float fovDegrees, float aspectRatio, float near, float far) {
    m_projMatrix = Matrix4x4::perspective(
        fovDegrees * 3.1415926f / 180.0f, // 转换为弧度
        aspectRatio,
        near,
        far
    );
}

Matrix4x4 Camera::getMVP(const Matrix4x4& modelMatrix) const {
    return m_projMatrix * m_viewMatrix * modelMatrix;
}

void Camera::setPosition(const vec3f& position) {
    m_position = position;
    updateViewMatrix();
}

void Camera::updateViewMatrix() {
    // 计算视图矩阵（lookAt 实现）
    vec3f forward = (m_target - m_position).normalized(); // 朝向向量
    vec3f right = forward.cross(m_up).normalized();       // 右向量
    vec3f up = right.cross(forward).normalized();         // 调整后的上向量

    // 视图矩阵的旋转部分
    Matrix4x4 rotation;
    rotation.m[0][0] = right.x;    rotation.m[0][1] = right.y;    rotation.m[0][2] = right.z;    rotation.m[0][3] = 0;
    rotation.m[1][0] = up.x;       rotation.m[1][1] = up.y;       rotation.m[1][2] = up.z;       rotation.m[1][3] = 0;
    rotation.m[2][0] = -forward.x; rotation.m[2][1] = -forward.y; rotation.m[2][2] = -forward.z; rotation.m[2][3] = 0;
    rotation.m[3][0] = 0;          rotation.m[3][1] = 0;          rotation.m[3][2] = 0;          rotation.m[3][3] = 1;

    // 平移部分
    Matrix4x4 translation = Matrix4x4::translation(-m_position.x, -m_position.y, -m_position.z);

    // 视图矩阵 = 旋转 * 平移
    m_viewMatrix = rotation * translation;
}