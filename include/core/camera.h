// include/core/camera.h
#pragma once
#include "math/matrix.h"
#include "math/vector.h"
#include "math/transform.h"

class Camera {
public:
    Camera(const vec3f& position = {0.0f, 0.0f, 3.0f}, float initialYaw = -90.0f, float initialPitch = 0.0f);
    
    void setPerspective(float fovDegrees, float aspectRatio, float near, float far);
    mat4 getMVP(const mat4& modelMatrix) const;
    
    const vec3f& getPosition() const { return m_transform.position; }
    const mat4& getViewMatrix() const { return m_viewMatrix; }
    const mat4& getProjectionMatrix() const { return m_projMatrix; }
    const Transform& getTransform() const { return m_transform; }

    void processKeyboardMovement(const vec3f& direction, float deltaTime, float speed);
    void processMouseMovement(float xoffset, float yoffset, float sensitivity, bool constrainPitch = true);

    void setPosition(const vec3f& position);
    void setPitchYaw(float pitch, float yaw);
    void setTransform(const Transform& transform);
    
    vec3f getForward() const;
    vec3f getRight() const;
    vec3f getUp() const; // Camera's local up

private:
    Transform m_transform;
    mat4 m_viewMatrix;
    mat4 m_projMatrix;
      
    float m_yaw;
    float m_pitch;

    const vec3f m_worldUp = {0.0f, 1.0f, 0.0f};

    void updateViewMatrix();
    void updateRotationAndVectors();
    void updateCameraVectors();
};

