// include/core/camera.h
#pragma once
#include "math/matrix.h"
#include "math/vector.h"

class Camera {
public:
    Camera(const vec3f& position, const vec3f& target, const vec3f& up);
    void setPerspective(float fovDegrees, float aspectRatio, float near, float far);
    mat4 getMVP(const mat4& modelMatrix) const;
    const vec3f& getPosition() const;
    const mat4& getViewMatrix() const;
    const mat4& getProjectionMatrix() const;
    void setPosition(const vec3f& position);

private:
    vec3f m_position;
    vec3f m_target;
    vec3f m_up;
    mat4 m_viewMatrix;
    mat4 m_projMatrix;
    void updateViewMatrix();
};

