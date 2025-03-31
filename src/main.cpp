#include <iostream>
#include "core/framebuffer.h"
#include "math/vector.h"
#include "math/matrix.h"

int main() {
    const int width = 800;
    const int height = 600;
    
    Framebuffer framebuffer(width, height);
    framebuffer.clear(Vector3<float>(0.2f, 0.2f, 0.2f));

    // Test drawing a red pixel
    framebuffer.setPixel(width/2, height/2, Vector3<float>(1.0f, 0.0f, 0.0f));

    // Save to file
    if (!framebuffer.saveToTGA("output.tga")) {
        std::cerr << "Failed to save image" << std::endl;
        return 1;
    }

    std::cout << "Rendered image saved to output.tga" << std::endl;
    return 0;
}
