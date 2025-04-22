// include/core/camera.h
#pragma once
#include "math/matrix.h"
#include "math/vector.h"
#include "math/transform.h"

class Camera {
public:
    Camera(const vec3f& position, const vec3f& target, const vec3f& up);
    void setPerspective(float fovDegrees, float aspectRatio, float near, float far);
    mat4 getMVP(const mat4& modelMatrix) const;
    const vec3f& getPosition() const { return m_transform.position; }
    const mat4& getViewMatrix() const { return m_viewMatrix; }
    const mat4& getProjectionMatrix() const { return m_projMatrix; }
    void setPosition(const vec3f& position);
    void setTarget(const vec3f& target);
    void setRotation(const vec3f& rotation); // Rotation in Euler angles (degrees)
    const vec3f& getTarget() const { return m_target; }
    const vec3f& getUp() const { return m_up; }
    const Transform& getTransform() const { return m_transform; }
    void setTransform(const Transform& transform);

private:
    Transform m_transform;
    vec3f m_target;
    vec3f m_up;
    mat4 m_viewMatrix;
    mat4 m_projMatrix;
    void updateViewMatrix();
};

