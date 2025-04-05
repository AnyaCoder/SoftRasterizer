#pragma once
#include "math/matrix.h"
#include "math/vector.h"

class Camera {
public:
    Camera(const Vector3<float>& position, const Vector3<float>& target, const Vector3<float>& up);
    void setPerspective(float fovDegrees, float aspectRatio, float near, float far);
    Matrix4x4 getMVP(const Matrix4x4& modelMatrix) const;
    void setPosition(const Vector3<float>& position);

private:
    Vector3<float> m_position;
    Vector3<float> m_target;
    Vector3<float> m_up;
    Matrix4x4 m_viewMatrix;
    Matrix4x4 m_projMatrix;
    void updateViewMatrix();
};

