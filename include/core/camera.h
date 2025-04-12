// include/core/camera.h
#pragma once
#include "math/matrix.h"
#include "math/vector.h"

class Camera {
public:
    Camera(const vec3f& position, const vec3f& target, const vec3f& up);
    void setPerspective(float fovDegrees, float aspectRatio, float near, float far);
    Matrix4x4 getMVP(const Matrix4x4& modelMatrix) const;
    const vec3f& getPosition() const;
    const Matrix4x4& getViewMatrix() const;
    const Matrix4x4& getProjectionMatrix() const;
    void setPosition(const vec3f& position);

private:
    vec3f m_position;
    vec3f m_target;
    vec3f m_up;
    Matrix4x4 m_viewMatrix;
    Matrix4x4 m_projMatrix;
    void updateViewMatrix();
};

