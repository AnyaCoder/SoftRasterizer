// include/core/texture/tga_texture.h
#pragma once
#include "core/texture/texture.h"

class TGATexture : public Texture {
public:
    bool load(const std::string& filename) override;
    vec3f sample(float u, float v) const override;
};