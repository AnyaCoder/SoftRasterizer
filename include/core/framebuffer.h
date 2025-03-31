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
    bool saveToTGA(const std::string& filename);
};
