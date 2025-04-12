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


    Model model;
    if (!model.loadFromObj("resources/obj/african_head.obj")) {
        std::cerr << "Failed to load model" << std::endl;
        return 1;
    }

    // Create Material for the model
    Material headMaterial(std::make_shared<BlinnPhongShader>());
    // Load texture into the model itself
    if (!headMaterial.loadDiffuseTexture("resources/diffuse/african_head_diffuse.tga")) {
        std::cerr << "Failed to load diffuse texture" << std::endl;
        // Decide if this is fatal - maybe render without texture?
        // return 1;
    }

    if (!headMaterial.loadNormalTexture("resources/normal_tangent/african_head_nm_tangent.tga")) {
        std::cerr << "Failed to load normal texture" << std::endl;
        // Decide if this is fatal - maybe render without texture?
        // return 1;
    }

    headMaterial.shininess = 100; // Set Blinn-Phong shininess
    headMaterial.specularColor = Vector3<float>(0.3f, 0.3f, 0.3f);


    // --- Setup Camera ---
    float near = 0.1f;
    float far = 100.0f;
    Camera camera(
        Vector3<float>(-2, 0, 3),   // Camera position (raised Y slightly)
        Vector3<float>(0, 0, 0),    // Target point
        Vector3<float>(0, 1, 0)     // Up direction
    );
    camera.setPerspective(45.0f, (float)width / height, near, far);

    Renderer renderer(framebuffer); // Create the renderer
    renderer.setCamera(camera);

    // --- Setup Lights ---
    std::vector<Light> lights;
    Light dirLight(LightType::DIRECTIONAL);
    dirLight.direction = Vector3<float>(0.707f, 0.0f, -0.707f).normalized();
    dirLight.color = Vector3<float>(1.0f, 1.0f, 1.0f);
    dirLight.intensity = 1.0f;
    lights.push_back(dirLight);
    renderer.setLights(lights);

    // --- Render Scene ---
    renderer.clear(Vector3<float>(0.5f, 0.5f, 0.5f)); // Dark grey background

    auto modelMatrix = mat4::identity();
    // Can apply model transformations here:
    // modelMatrix = mat4::translation(0, 0.5, 0) * mat4::rotationY(3.14159f / 3.0);

    // Pass model, its transform, and its material to the renderer
    renderer.drawModel(model, modelMatrix, headMaterial);

    // --- Post-processing & Save ---
    framebuffer.flipVertical(); // Flip if needed for TGA convention

    if (!framebuffer.saveToTGA("output_shader.tga")) {
        std::cerr << "Failed to save image" << std::endl;
        return 1;
    }

    std::cout << "Rendered image saved to output_shader.tga" << std::endl;
    return 0;
}