#pragma once
#include <vector>
#include <string>
#include "math/vector.h"

class Framebuffer {
public:
    int width;
    int height;
    std::vector<Vector3<float>> pixels;
    std::vector<float> zBuffer;  // Depth buffer

    Framebuffer(int w, int h);
    void clearZBuffer();
    void clear(const Vector3<float>& color);
    void setPixel(int x, int y, const Vector3<float>& color, float depth = 0.0f);
    void drawLine(int x0, int y0, int x1, int y1, const Vector3<float>& color);
    void drawTriangle(int x0, int y0, float z0 = 0.0f,
                     int x1 = 0, int y1 = 0, float z1 = 0.0f,
                     int x2 = 0, int y2 = 0, float z2 = 0.0f,
                     const Vector3<float>& color = Vector3<float>());
    bool saveToTGA(const std::string& filename);
    
    void flipHorizontal();
    void flipVertical();
private:
    int interpolate(int x1, int y1, int x2, int y2, int y);
    
};
