// core/texture/texture.cpp
#include "core/texture/texture.h"

vec3f Texture::sampleBilinear(const MipLevel& level, float u, float v)  {
    if (level.pixels.empty() || level.width <= 0 || level.height <= 0) {
        return vec3f(1.0f, 0.0f, 1.0f); // Error color
    }

    // Wrap coordinates
    u = u - std::floor(u);
    v = v - std::floor(v);

    // Calculate exact texture coordinates (center of pixel is at .5)
    float tx = u * level.width - 0.5f;
    float ty = v * level.height - 0.5f;

    // Integer pixel coordinates
    int x0 = static_cast<int>(std::floor(tx));
    int y0 = static_cast<int>(std::floor(ty));

    // Interpolation weights
    float u_frac = tx - x0;
    float v_frac = ty - y0;

    // Sample 4 neighbors (with clamping)
    auto getPixelClamped = [&](int x, int y) -> const vec3f& {
        x = std::max(0, std::min(level.width - 1, x));
        y = std::max(0, std::min(level.height - 1, y));
        return level.pixels[y * level.width + x];
    };

    const vec3f& c00 = getPixelClamped(x0, y0);
    const vec3f& c10 = getPixelClamped(x0 + 1, y0);
    const vec3f& c01 = getPixelClamped(x0, y0 + 1);
    const vec3f& c11 = getPixelClamped(x0 + 1, y0 + 1);

    // Bilinear interpolation
    vec3f top = c00 * (1.0f - u_frac) + c10 * u_frac;
    vec3f bottom = c01 * (1.0f - u_frac) + c11 * u_frac;
    return top * (1.0f - v_frac) + bottom * v_frac;
}