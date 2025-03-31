#include <iostream>
#include "core/framebuffer.h"
#include "math/vector.h"
#include "math/matrix.h"

int main() {
    const int width = 800;
    const int height = 600;
    
    Framebuffer framebuffer(width, height);
    framebuffer.clear(Vector3<float>(0.2f, 0.2f, 0.2f));

    // Test drawing lines in different directions
    framebuffer.drawLine(100, 100, 700, 100, Vector3<float>(1.0f, 0.0f, 0.0f)); // Horizontal
    framebuffer.drawLine(100, 150, 700, 500, Vector3<float>(0.0f, 1.0f, 0.0f)); // Diagonal
    framebuffer.drawLine(400, 100, 400, 500, Vector3<float>(0.0f, 0.0f, 1.0f)); // Vertical
    framebuffer.drawLine(700, 100, 500, 150, Vector3<float>(1.0f, 1.0f, 0.0f)); // Reverse diagonal

    // Save to file
    if (!framebuffer.saveToTGA("output.tga")) {
        std::cerr << "Failed to save image" << std::endl;
        return 1;
    }

    std::cout << "Rendered image saved to output.tga" << std::endl;
    return 0;
}
