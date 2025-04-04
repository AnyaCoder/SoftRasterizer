#include <iostream>
#include "core/framebuffer.h"
#include "core/model.h"
#include "math/vector.h"
#include "math/matrix.h"

int main() {
    const int width = 800;
    const int height = 800;
    
    Framebuffer framebuffer(width, height);
    framebuffer.clear(Vector3<float>(0.5f, 0.5f, 0.5f));
    framebuffer.clearZBuffer();

    Model model;
    if (!model.loadFromObj("resources/obj/african_head.obj")) {
        std::cerr << "Failed to load model" << std::endl;
        return 1;
    }

    // Load diffuse texture
    if (!model.loadDiffuseTexture("resources/diffuse/african_head_diffuse.tga")) {
        std::cerr << "Failed to load texture" << std::endl;
        return 1;
    }

    float near = 0.1f;
    float far = 100.0f;

    // 设置变换矩阵
    Matrix4x4 modelMatrix = Matrix4x4::identity();
    Matrix4x4 viewMatrix = Matrix4x4::translation(0, 0, -3); // 相机向后移动
    Matrix4x4 projectionMatrix = Matrix4x4::perspective(
        45.0f * 3.1415926f / 180.0f, // FOV
        (float)width/height,   // 宽高比
        near,                 // 近裁剪面
        far                // 远裁剪面
    );
    
    // 组合变换矩阵
    Matrix4x4 mvp = projectionMatrix * viewMatrix * modelMatrix;

    // 渲染
    model.renderSolid(framebuffer, near, far, mvp, Vector3<float>(1.0f, 1.0f, 1.0f), 
                       Vector3<float>(0.0f, 0.0f, 1.0f));
    
    framebuffer.flipVertical();
    // Save to file
    if (!framebuffer.saveToTGA("output.tga")) {
        std::cerr << "Failed to save image" << std::endl;
        return 1;
    }

    std::cout << "Rendered image saved to output.tga" << std::endl;
    return 0;
}
