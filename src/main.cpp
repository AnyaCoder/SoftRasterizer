#include <iostream>
#include "core/sdl_app.h"
#include "core/scene.h"

int main(int argc, char* argv[]) {
    const int width = 800;
    const int height = 800;

    // Initialize SDL application
    SDLApp app(width, height, "Software Rasterizer");
    if (!app.initialize()) {
        std::cerr << "Failed to initialize SDLApp" << std::endl;
        return 1;
    }

    // Initialize scene
    Scene scene(width, height);
    if (!scene.loadFromYAML("scenes/scene.yaml")) {
        std::cerr << "Failed to load scene, using default" << std::endl;
    }

    // Run the application with a render callback
    app.run([&scene](float deltaTime) -> const Framebuffer& {
        scene.update(deltaTime);
        scene.render();
        return scene.getFramebuffer();
    });

    std::cout << "Exiting..." << std::endl;
    return 0;
}