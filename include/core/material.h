// include/core/material.h
#pragma once
#include "math/vector.h"
#include "core/resource_manager.h"
#include "core/texture/texture.h"
#include "core/texture/tga_texture.h"
#include "core/texture/dds_texture.h"
#include "core/shader.h"
#include <memory>

class Shader;

class Material {
public:
    
    Material() = default;

    vec3f ambientColor = {0.1f, 0.1f, 0.1f};
    vec3f diffuseColor = {0.8f, 0.8f, 0.8f};
    vec3f specularColor = {0.5f, 0.5f, 0.5f};
    int shininess = 1 << 6;

    std::shared_ptr<Texture> diffuseTexture;
    std::shared_ptr<Texture> normalTexture;
    std::shared_ptr<Texture> aoTexture;
    std::shared_ptr<Texture> specularTexture;
    std::shared_ptr<Texture> glossTexture;
    
    std::shared_ptr<Shader> shader;

private:
    friend class ResourceManager;
};