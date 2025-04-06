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

    // Load texture into the model itself
    if (!model.loadDiffuseTexture("resources/diffuse/african_head_diffuse.tga")) {
        std::cerr << "Failed to load texture" << std::endl;
        // Decide if this is fatal - maybe render without texture?
        // return 1;
    }

    // Create Material for the model
    Material headMaterial;
    headMaterial.shininess = 100; // Set Blinn-Phong shininess
    headMaterial.specularColor = Vector3<float>(0.6f, 0.6f, 0.6f);
    // Assign model's texture to the material
    headMaterial.diffuseTexture = model.getDiffuseTexture();


    // --- Setup Camera ---
    float near = 0.1f;
    float far = 100.0f;
    Camera camera(
        Vector3<float>(0, 1, 3),   // Camera position (raised Y slightly)
        Vector3<float>(0, 0, 0),    // Target point
        Vector3<float>(0, 1, 0)     // Up direction
    );
    camera.setPerspective(45.0f, (float)width / height, near, far);

    Renderer renderer(framebuffer); // Create the renderer
    renderer.setCamera(camera);

    // --- Setup Lights ---
    std::vector<Light> lights;
    // Light 1: Directional Light (like sun)
    Light dirLight(LightType::DIRECTIONAL);
    dirLight.direction = Vector3<float>(0.707f, 0.0f, -0.707f).normalized(); // From top-right-front
    dirLight.color = Vector3<float>(1.0f, 1.0f, 1.0f);
    dirLight.intensity = 1.0f;
    lights.push_back(dirLight);

    // Light 2: Point Light (example)
    // Light pointLight(LightType::POINT);
    // pointLight.position = Vector3<float>(-2.5f, 2.0f, 2.0f); // Position in world space
    // pointLight.color = Vector3<float>(0.2f, 0.7f, 0.2f); // Greenish light
    // pointLight.intensity = 1.5f;
    // Add attenuation factors if implemented in shader
    // lights.push_back(pointLight);

    renderer.setLights(lights); // Pass lights to renderer


    // --- Setup Shader ---
    auto shader = std::make_shared<BlinnPhongShader>();
    // Set global ambient light level (can be part of renderer/scene settings too)
    shader->uniform_AmbientLight = Vector3<float>(0.15f, 0.15f, 0.15f);
    renderer.setShader(shader);


    // --- Render Scene ---
    renderer.clear(Vector3<float>(0.5f, 0.5f, 0.5f)); // Dark grey background

    auto modelMatrix = Matrix4x4::identity();
    // Can apply model transformations here:
    // modelMatrix = Matrix4x4::translation(0, -0.5, 0) * Matrix4x4::rotationY(M_PI / 4.0);

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