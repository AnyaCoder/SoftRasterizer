---
title: Camera Implementation
date: 2025-04-05
categories:
  - Computer Graphics
tags:
  - Rendering
  - Camera System
  - View Matrix
---

## 概述
本文档详细记录了在 SoftRasterizer 项目中实现 `Camera` 类的全过程，包括其设计、核心代码、应用方式以及验证方法。通过该类，我们实现了灵活的相机控制，支持 OpenGL 风格的视图变换。

## 核心实现

### 1. Camera 类定义
```cpp
// camera.h
#pragma once
#include "math/matrix.h"
#include "math/vector.h"

class Camera {
public:
    Camera(const vec3f& position, const vec3f& target, const vec3f& up);
    void setPerspective(float fovDegrees, float aspectRatio, float near, float far);
    mat4 getMVP(const mat4& modelMatrix) const;
    void setPosition(const vec3f& position);

private:
    vec3f m_position;  // 相机位置
    vec3f m_target;    // 目标点
    vec3f m_up;        // 上方向
    mat4 m_viewMatrix;     // 视图矩阵
    mat4 m_projMatrix;     // 投影矩阵
    void updateViewMatrix();    // 更新视图矩阵
};
```

### 2. Camera 类实现

```cpp
// camera.cpp
#include "core/camera.h"

Camera::Camera(const vec3f& position, const vec3f& target, const vec3f& up)
    : m_position(position), m_target(target), m_up(up) {
    updateViewMatrix();
    m_projMatrix = mat4::identity();
}

void Camera::setPerspective(float fovDegrees, float aspectRatio, float near, float far) {
    m_projMatrix = mat4::perspective(fovDegrees * 3.1415926f / 180.0f, aspectRatio, near, far);
}

mat4 Camera::getMVP(const mat4& modelMatrix) const {
    return m_projMatrix * m_viewMatrix * modelMatrix;
}

void Camera::setPosition(const vec3f& position) {
    m_position = position;
    updateViewMatrix();
}

void Camera::updateViewMatrix() {
    vec3f forward = (m_target - m_position).normalized();
    vec3f right = forward.cross(m_up).normalized();
    vec3f up = right.cross(forward).normalized();

    mat4 rotation;
    rotation.m[0][0] = right.x;    rotation.m[0][1] = right.y;    rotation.m[0][2] = right.z;    rotation.m[0][3] = 0;
    rotation.m[1][0] = up.x;       rotation.m[1][1] = up.y;       rotation.m[1][2] = up.z;       rotation.m[1][3] = 0;
    rotation.m[2][0] = -forward.x; rotation.m[2][1] = -forward.y; rotation.m[2][2] = -forward.z; rotation.m[2][3] = 0;
    rotation.m[3][0] = 0;          rotation.m[3][1] = 0;          rotation.m[3][2] = 0;          rotation.m[3][3] = 1;

    mat4 translation = mat4::translation(-m_position.x, -m_position.y, -m_position.z);
    m_viewMatrix = rotation * translation;
}
```

### 3. 主循环集成
```cpp
// main.cpp
int main() {
    const int width = 800, height = 800;
    Framebuffer framebuffer(width, height);
    framebuffer.clear(vec3f(0.5f, 0.5f, 0.5f));
    framebuffer.clearZBuffer();

    Model model;
    if (!model.loadFromObj("resources/obj/african_head.obj") || 
        !model.loadDiffuseTexture("resources/diffuse/african_head_diffuse.tga")) {
        std::cerr << "Failed to load model or texture" << std::endl;
        return 1;
    }

    float near = 0.1f, far = 100.0f;
    Camera camera(vec3f(0, 0, 3), vec3f(0, 0, 0), vec3f(0, 1, 0));
    camera.setPerspective(45.0f, (float)width / height, near, far);

    mat4 modelMatrix = mat4::identity();
    mat4 mvp = camera.getMVP(modelMatrix);

    model.renderSolid(framebuffer, near, far, mvp, vec3f(1.0f, 1.0f, 1.0f), vec3f(0.0f, 0.0f, -1.0f));

    framebuffer.flipVertical();
    if (!framebuffer.saveToTGA("output.tga")) {
        std::cerr << "Failed to save image" << std::endl;
        return 1;
    }
    std::cout << "Rendered image saved to output.tga" << std::endl;
    return 0;
}
```

### 技术要点

- **坐标系**：采用右手坐标系，+Z 为屏幕外，相机默认朝向由目标点决定。

- **视图矩阵**：通过 lookAt 方法生成，先平移到相机原点，再旋转到相机坐标系。

- **退化处理**：当 forward 和 up 平行时，需调整 up（如从 +Y 看 -Y 时用 -Z）。

- **投影矩阵**：支持透视投影，FOV 转换为弧度，确保与 OpenGL 一致。

### 应用与验证

#### 应用场景
- **正面视角**：相机位于 (0, 0, 3)，朝向 (0, 0, 0)，光照从 +Z 到 -Z，看到 african_head.obj 正面。
- **灵活调整**：通过 setPosition 和目标点调整相机位置和朝向。

#### 验证方法
- **正面验证**：
  - 配置：Camera(vec3f(0, 0, 3), vec3f(0, 0, 0), vec3f(0, 1, 0))
  - 光照：(0, 0, -1)
  - 预期：看到模型正面。

- **背面验证**：
  - 配置：Camera(vec3f(0, 0, -3), vec3f(0, 0, 0), vec3f(0, 1, 0))
  - 光照：(0, 0, 1)
  - 预期：看到模型背面。

- **侧面验证**：
  - 配置：Camera(vec3f(3, 0, 0), vec3f(0, 0, 0), vec3f(0, 1, 0))
  - 光照：(-1, 0, 0)
  - 预期：看到模型右侧。

- **顶部验证**：
  - 配置：Camera(vec3f(0, 3, 0), vec3f(0, 0, 0), vec3f(0, 0, -1))
  - 光照：(0, -1, 0)
  - 预期：看到模型顶部，无退化。

- **左前方验证**：
  - 配置：Camera(vec3f(-2, 0, 3), vec3f(0, 0, 0), vec3f(0, 1, 0))
  - 光照：(0.707, 0, -0.707)
  - 预期：看到模型左前方。
### 总结

通过实现 Camera 类，我们成功支持了灵活的相机控制，能够正确渲染 african_head.obj 的各个角度。验证过程确认了视图矩阵、光照和坐标系的一致性，确保了渲染结果符合预期。
