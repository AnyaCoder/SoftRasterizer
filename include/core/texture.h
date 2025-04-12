// include/core/texture.h
#pragma once
#include "math/vector.h"
#include <vector>
#include <string>

class Texture {
public:
    int width;
    int height;
    std::vector<vec3f> pixels;

    bool loadFromTGA(const std::string& filename);
    vec3f sample(float u, float v) const;
    bool empty() const {
        return pixels.empty() || width == 0 || height == 0;
    }
};
