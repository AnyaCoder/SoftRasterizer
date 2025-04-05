#include "core/texture.h"
#include "../io/tga_writer.h"

bool Texture::loadFromTGA(const std::string& filename) {
    std::vector<unsigned char> data;
    if (!loadTGA(filename, width, height, data)) {
        return false;
    }

    pixels.resize(width * height);
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = (y * width + x) * 3;
            pixels[y * width + x] = Vector3<float>(
                data[idx] / 255.0f,
                data[idx+1] / 255.0f,
                data[idx+2] / 255.0f
            );
        }
    }
    return true;
}

Vector3<float> Texture::sample(float u, float v) const {
    // Wrap texture coordinates
    u = u - floorf(u);
    v = v - floorf(v);
    
    // Convert to pixel coordinates
    int x = static_cast<int>(u * (width - 1));
    int y = static_cast<int>(v * (height - 1));
    
    // Clamp coordinates
    x = std::max(0, std::min(width-1, x));
    y = std::max(0, std::min(height-1, y));
    
    return pixels[y * width + x];
}
