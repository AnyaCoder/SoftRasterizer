// include/core/material.h
#pragma once

#include "math/vector.h"
#include "texture.h"
#include "shader.h"

class Shader;

struct Material {
    vec3f ambientColor = {0.1f, 0.1f, 0.1f};
    vec3f diffuseColor = {0.8f, 0.8f, 0.8f};
    vec3f specularColor = {0.5f, 0.5f, 0.5f};
    Texture diffuseTexture;
    Texture normalTexture;
    int shininess = 32;

    std::shared_ptr<Shader> shader;
    Material(std::shared_ptr<Shader> sh = nullptr) : shader(sh) {}
    
    bool loadDiffuseTexture(const std::string& filename) {
        return diffuseTexture.loadFromTGA(filename);
    }

    bool loadNormalTexture(const std::string& filename) {
        return normalTexture.loadFromTGA(filename);
    }
    
};