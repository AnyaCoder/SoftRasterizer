// src/main.cpp
#include <iostream>
#include "core/sdl_app.h"
#include "core/scene.h"

int main(int argc, char* argv[]) {
    const int width = 800;
    const int height = 800;
    const std::string title = "Software Rasterizer (Refactored)";

    std::cout << "Starting application..." << std::endl;

    SDLApp app(width, height, title); // Construction and initialization happens here

    if (!app.initialize()) { // Initialization now part of try block
        std::cerr << "Failed to initialize SDLApp" << std::endl;
        return 1;
    }
    app.run();
    
    std::cout << "Exiting..." << std::endl;

    return 0;
}