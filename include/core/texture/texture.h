// include/core/texture/texture.h
#pragma once
#include "math/vector.h"
#include <vector>
#include <string>

class Texture {
public:
    virtual ~Texture() = default;

    virtual bool load(const std::string& filename) = 0;
    virtual vec3f sample(float u, float v) const = 0;

    bool empty() const {
        return pixels.empty() || width == 0 || height == 0;
    }

    int width = 0;
    int height = 0;
    std::vector<vec3f> pixels;

protected:
    Texture() = default;
};