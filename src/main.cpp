#include <iostream>
#include "core/framebuffer.h"
#include "core/model.h"
#include "math/vector.h"
#include "math/matrix.h"

int main() {
    const int width = 800;
    const int height = 800;
    
    Framebuffer framebuffer(width, height);
    framebuffer.clear(Vector3<float>(0.1f, 0.1f, 0.1f));

    Model model;
    if (!model.loadFromObj("obj/african_head.obj")) {
        std::cerr << "Failed to load model" << std::endl;
        return 1;
    }

    // Render model in white wireframe
    model.renderSolid(framebuffer, Vector3<float>(1.0f, 1.0f, 1.0f), Vector3<float>(0.0f, 0.0f, 1.0f));
    
    framebuffer.flipVertical();
    // Save to file
    if (!framebuffer.saveToTGA("output.tga")) {
        std::cerr << "Failed to save image" << std::endl;
        return 1;
    }

    std::cout << "Rendered image saved to output.tga" << std::endl;
    return 0;
}
