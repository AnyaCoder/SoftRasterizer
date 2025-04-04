#pragma once
#include "math/vector.h"
#include <vector>
#include <string>

class Texture {
public:
    int width;
    int height;
    std::vector<Vector3<float>> pixels;

    bool loadFromTGA(const std::string& filename);
    Vector3<float> sample(float u, float v) const;
    bool empty() const {
        return pixels.empty() || width == 0 || height == 0;
    }
};
