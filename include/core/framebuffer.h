#pragma once
#include <vector>
#include <string>
#include "math/vector.h"

class Framebuffer {
public:
    int width;
    int height;
    std::vector<Vector3<float>> pixels;

    Framebuffer(int w, int h);
    void clear(const Vector3<float>& color);
    void setPixel(int x, int y, const Vector3<float>& color);
    void drawLine(int x0, int y0, int x1, int y1, const Vector3<float>& color);
    void drawTriangle(int x0, int y0, int x1, int y1, int x2, int y2, const Vector3<float>& color);
    bool saveToTGA(const std::string& filename);
    
    void flipHorizontal();
    void flipVertical();
private:
    int interpolate(int x1, int y1, int x2, int y2, int y);
    
};
