#pragma once
#include <vector>
#include <string>
#include "math/vector.h"
#include "core/texture.h"
#include "core/vertex.h"

class Framebuffer {
public:
    Framebuffer(int w, int h);

    void clearZBuffer();
    void clear(const vec3f& color);

    // Modified setPixel takes depth in [0, 1] range (0=near, 1=far)
    void setPixel(int x, int y, const vec3f& color, float depth = 0.0f);

    // Getter for depth buffer value
    float getDepth(int x, int y) const { 
        if (x >= 0 && x < width && y >= 0 && y < height) {
            return zBuffer[y * width + x];
        }
        return 1.0f; // Return farthest depth if outside bounds
    }

    int getWidth() const { return width; }
    int getHeight() const { return height; }

    bool saveToTGA(const std::string& filename);
    
    void flipHorizontal();
    void flipVertical();

private:
    int width;
    int height;
    std::vector<vec3f> pixels;
    std::vector<float> zBuffer;  // Depth buffer
    void drawScanlines(int yStart, int yEnd, 
        const Vertex& vStartA, const Vertex& vEndA,
        const Vertex& vStartB, const Vertex& vEndB,
        const vec3f& color, const Texture& texture);
        
};
