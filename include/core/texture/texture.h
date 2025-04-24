// include/core/texture/texture.h
#pragma once
#include "math/vector.h"
#include <vector>
#include <string>

class Texture {
public:
    struct MipLevel {
        int width = 0;
        int height = 0;
        std::vector<vec3f> pixels;
    };

    virtual ~Texture() = default;

    virtual vec3f sample(float u, float v, const vec2f& ddx, const vec2f& ddy) const = 0;

    bool empty() const {
        return mipLevels.empty() || mipLevels[0].width == 0 || mipLevels[0].height == 0;
    }

    int getWidth() const { return mipLevels.empty() ? 0 : mipLevels[0].width; }
    int getHeight() const { return mipLevels.empty() ? 0 : mipLevels[0].height; }
    size_t getNumLevels() const { return mipLevels.size(); }
    const MipLevel& getLevel(size_t level) const { return mipLevels[level]; }

protected:
    Texture() = default;
    virtual bool load(const std::string& filename) = 0;
    static vec3f sampleBilinear(const MipLevel& level, float u, float v);
    std::vector<MipLevel> mipLevels;
    friend class ResourceManager; 
};