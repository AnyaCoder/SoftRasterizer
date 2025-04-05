#pragma once
#include <vector>
#include <string>
#include "math/vector.h"
#include "core/texture.h"
#include "core/vertex.h"

class Framebuffer {
public:
    int width;
    int height;
    std::vector<vec3f> pixels;
    std::vector<float> zBuffer;  // Depth buffer

    Framebuffer(int w, int h);
    void clearZBuffer();
    void clear(const vec3f& color);
    void setPixel(int x, int y, const vec3f& color, float depth = 0.0f);
    void drawLine(int x0, int y0, int x1, int y1, const vec3f& color);
    void drawTriangle(Vertex v0, Vertex v1, Vertex v2,
                     const vec3f& color, 
                     const Texture& texture);
    bool saveToTGA(const std::string& filename);
    
    void flipHorizontal();
    void flipVertical();

private:
    void drawScanlines(int yStart, int yEnd, 
        const Vertex& vStartA, const Vertex& vEndA,
        const Vertex& vStartB, const Vertex& vEndB,
        const vec3f& color, const Texture& texture);

    template<typename T, typename U>
    T interpolate(T x1, U y1, T x2, U y2, U y) {
        if (y1 == y2) return x1;
        return x1 + static_cast<T>((x2 - x1) * (y - y1) / (y2 - y1));
    }
        
};
