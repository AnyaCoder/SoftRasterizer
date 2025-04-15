#include <iostream>
#include <vector>
#include <memory> // For std::shared_ptr

#include "core/framebuffer.h"
#include "core/model.h"
#include "core/camera.h"
#include "core/renderer.h"
#include "core/shader.h"
#include "core/blinn_phong_shader.h"
#include "core/light.h"
#include "core/material.h"
#include "math/vector.h"
#include "math/matrix.h"

int main() {
    const int width = 800;
    const int height = 800;

    Framebuffer framebuffer(width, height);

    Model headModel;
    headModel.loadFromObj("resources/obj/african_head.obj");

    Material headMaterial(std::make_shared<BlinnPhongShader>());
    headMaterial.loadDiffuseTexture("resources/diffuse/african_head_diffuse.tga");
    headMaterial.loadNormalTexture("resources/normal_tangent/african_head_nm_tangent.tga");
    headMaterial.loadSpecularTexture("resources/spec/african_head_spec.tga");

    Model eyeInnerModel;
    eyeInnerModel.loadFromObj("resources/obj/african_head_eye_inner.obj");

    Material eyeInnerMaterial(std::make_shared<BlinnPhongShader>());
    eyeInnerMaterial.loadDiffuseTexture("resources/diffuse/african_head_eye_inner_diffuse.tga");
    eyeInnerMaterial.loadNormalTexture("resources/normal_tangent/african_head_eye_inner_nm_tangent.tga");
    eyeInnerMaterial.loadSpecularTexture("resources/spec/african_head_eye_inner_spec.tga");

    // --- Setup Camera ---
    float near = 0.1f;
    float far = 100.0f;
    Camera camera(
        vec3f(0, 1, 3),   // Camera position (raised Y slightly)
        vec3f(0, 0, 0),    // Target point
        vec3f(0, 1, 0)     // Up direction
    );
    camera.setPerspective(45.0f, (float)width / height, near, far);

    Renderer renderer(framebuffer); // Create the renderer
    renderer.setCamera(camera);

    // --- Setup Lights ---
    std::vector<Light> lights;
    Light dirLight(LightType::DIRECTIONAL);
    dirLight.direction = vec3f(0.707f, 0.0f, -0.707f).normalized();
    dirLight.color = vec3f(1.0f, 1.0f, 1.0f);
    dirLight.intensity = 1.0f;
    lights.push_back(dirLight);
    renderer.setLights(lights);

    // --- Render Scene ---
    renderer.clear(vec3f(0.5f, 0.5f, 0.5f)); // Dark grey background

    Transform modelTransform;
    modelTransform.setPosition({0.0f, 0.0f, 0.0f});
    modelTransform.setRotationEulerZYX({-30.0f, 0.0f, 0.0f});
    modelTransform.setScale({1.0f, 1.0f, 1.0f});

    // Pass model, its transform, and its material to the renderer
    renderer.drawModel(headModel, modelTransform, headMaterial);
    renderer.drawModel(eyeInnerModel, modelTransform, eyeInnerMaterial);

    // --- Post-processing & Save ---
    framebuffer.flipVertical(); // Flip if needed for TGA convention

    if (!framebuffer.saveToTGA("output_shader.tga")) {
        std::cerr << "Failed to save image" << std::endl;
        return 1;
    }

    std::cout << "Rendered image saved to output_shader.tga" << std::endl;
    return 0;
}