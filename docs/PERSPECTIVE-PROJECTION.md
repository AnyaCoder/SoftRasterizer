---
title: "Perspective Projection"
date: 2025-04-05 10:00:00
categories:
  - Graphics
  - Soft Rasterizer
tags:
  - C++
  - Rendering
  - Perspective Projection
mathjax: true
comments: true
---

# 实现软光栅化中的透视投影：从基础渲染到深度测试优化

在开发软光栅化渲染器时，透视投影是实现真实感渲染的关键一步。本文基于一次代码修改（`git diff`），详细讲述如何将一个基础的模型渲染系统升级为支持透视投影的渲染管线，包括矩阵变换、深度处理和透视校正的实现过程。

## 背景

最初的渲染代码（`src/main.cpp`）使用简单的屏幕空间投影，直接将模型的顶点映射到帧缓冲区，没有考虑透视效果和深度缓冲的正确性：

```cpp
model.renderSolid(framebuffer, vec3f(1.0f, 1.0f, 1.0f), vec3f(0.0f, 0.0f, 1.0f));
```

目标是引入透视投影，使远处的物体变小，并通过深度测试实现正确的遮挡关系。以下是实现过程的步骤。

## 步骤 1：引入变换矩阵

在 `src/main.cpp` 中，我们添加了模型、视图和投影矩阵，用于将顶点从模型空间变换到裁剪空间：

```cpp
float near = 0.1f;
float far = 100.0f;

mat4 modelMatrix = mat4::identity();
mat4 viewMatrix = mat4::translation(0, 0, -3); // 相机后移
mat4 projectionMatrix = mat4::perspective(
    45.0f * 3.1415926f / 180.0f, // FOV
    (float)width/height,          // 宽高比
    near,                         // 近裁剪面
    far                           // 远裁剪面
);

mat4 mvp = projectionMatrix * viewMatrix * modelMatrix;
```

- 模型矩阵：保持不变（identity），后续可添加旋转或缩放。
- 视图矩阵：将相机向后移动 3 个单位，模拟观察者的位置。
- 投影矩阵：使用透视投影，定义视锥体（FOV 为 45°）。
- MVP 矩阵：组合三者，用于顶点变换。

渲染调用改为：

```cpp
model.renderSolid(framebuffer, near, far, mvp, vec3f(1.0f, 1.0f, 1.0f), vec3f(0.0f, 0.0f, 1.0f));
```

## 步骤 2：顶点变换与透视除法

在 `src/core/model.cpp` 中，`renderSolid` 方法从简单的屏幕映射升级为完整的透视投影管线：

### 2.1 顶点变换

将顶点从模型空间变换到裁剪空间：

```cpp
vec4f clip_coords[3];
vec3f world_coords[3];
float w_values[3];

for (int j = 0; j < 3; j++) {
    world_coords[j] = vertices[face[j]];
    vec4f v(world_coords[j], 1.0f);
    clip_coords[j] = mvp * v;
    w_values[j] = clip_coords[j].w;
}
```

- 使用齐次坐标（w=1）进行矩阵乘法。
- 存储 w 值，用于后续透视除法和校正。

### 2.2 简单裁剪检查

丢弃完全在近裁剪面外的三角形：

```cpp
if (clip_coords[0].z < -w_values[0] &&
    clip_coords[1].z < -w_values[1] &&
    clip_coords[2].z < -w_values[2]) {
    continue;
}
```

### 2.3 透视除法与视口变换

将裁剪空间坐标转换为 NDC（标准化设备坐标），并映射到屏幕空间：

```cpp
Vertex vertices[3];
for (int j = 0; j < 3; j++) {
    if (w_values[j] <= 0) continue;
    float invW = 1.0f / w_values[j];
    vec3f ndc(
        clip_coords[j].x * invW,
        clip_coords[j].y * invW,
        clip_coords[j].z * invW
    );
    vertices[j].x = (ndc.x + 1.0f) * fb.width * 0.5f;
    vertices[j].y = (ndc.y + 1.0f) * fb.height * 0.5f;
    // ... 深度映射 ...
    vertices[j].u = tex_coords[j].x * invW;
    vertices[j].v = tex_coords[j].y * invW;
    vertices[j].w = invW;
}
```

- 透视除法：除以 w 得到 NDC。
- 视口变换：将 [-1,1] 范围映射到屏幕坐标。

## 步骤 3：深度处理优化

### 3.1 深度值映射

将视空间的 z 值映射到 [0,1] 范围，靠近相机为 0，远离为 1：

```cpp
float zEye = clip_coords[j].z;
if (w_values[j] != 0) {
    vertices[j].z = (1.0f - (near * far / zEye * invW + near) / (far - near)) * 0.5f + 0.5f;
} else {
    vertices[j].z = 1.0f;
}
```

- 使用非线性映射，确保透视效果下的深度分布正确。
- 反转逻辑，使更近的点得到更小的深度值。

### 3.2 深度测试调整

在 `src/core/framebuffer.cpp` 中，将深度测试改为 "小于" 测试：

```cpp
if (depth < zBuffer[index]) {  // z值越小表示越近
    zBuffer[index] = depth;
    pixels[index] = color;
}
```

初始化时将深度缓冲区清为最大值：

```cpp
Framebuffer::Framebuffer(int w, int h) : width(w), height(h), pixels(w * h), zBuffer(w * h, std::numeric_limits<float>::max()) {}
void Framebuffer::clearZBuffer() {
    std::fill(zBuffer.begin(), zBuffer.end(), std::numeric_limits<float>::max());
}
```

## 步骤 4：透视校正插值

在 `drawScanlines` 中添加透视校正插值，确保纹理随深度正确变化：

```cpp
float wa = interpolate<float, int>(vStartA.w, vStartA.y, vEndA.w, vEndA.y, y);
float wb = interpolate<float, int>(vStartB.w, vStartB.y, vEndB.w, vEndB.y, y);
// ...
float w = wa + (wb - wa) * t;
if (useTexture && w != 0) {
    float invW = 1.0f / w;
    float u = (ua + (ub - ua) * t) * invW;
    float v = (va + (vb - va) * t) * invW;
    finalColor = texture.sample(u, v) * color;
}
```

- 插值 1/w 而不是直接插值纹理坐标。
- 在最终采样前除以 w，实现透视校正。

## 成果与反思

通过以上步骤，我们实现了：
- 透视投影：物体随距离变小。
- 深度测试：靠近相机的物体遮挡远处的物体。
- 纹理校正：纹理随视角正确变形。

然而，这仍是一个简化实现。未来的改进可以包括：
- 更复杂的裁剪算法（处理跨越裁剪面的三角形）。
- 支持透视投影下的背面剔除。
- 优化性能（如 SIMD 加速）。

代码已成功渲染出带有透视效果的非洲人头模型，保存为 `output.tga`。这是一个软光栅化学习过程中的重要里程碑！
