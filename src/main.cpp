#include <iostream>
#include "core/framebuffer.h"
#include "core/model.h"
#include "core/camera.h"
#include "math/vector.h"
#include "math/matrix.h"

int main() {
    const int width = 800;
    const int height = 800;
    
    Framebuffer framebuffer(width, height);
    framebuffer.clear(vec3f(0.5f, 0.5f, 0.5f));
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

    // 设置相机
    Camera camera(
        vec3f(-2, 0, 3),  // 相机位置
        vec3f(0, 0, 0),   // 目标点（朝向 -Z）
        vec3f(0, 1, 0)  // 上方向
    );
    camera.setPerspective(45.0f, (float)width / height, near, far);

    auto modelMatrix = Matrix4x4::identity();
    Matrix4x4 mvp = camera.getMVP(modelMatrix);
    auto lightColor = vec3f(1.0f, 1.0f, 1.0f);
    auto lightDir = vec3f(0.707f, 0.0f, -0.707f);
    // 渲染
    model.renderSolid(framebuffer, near, far, mvp, lightColor, lightDir);
    
    framebuffer.flipVertical();
    // Save to file
    if (!framebuffer.saveToTGA("output.tga")) {
        std::cerr << "Failed to save image" << std::endl;
        return 1;
    }

    std::cout << "Rendered image saved to output.tga" << std::endl;
    return 0;
}
