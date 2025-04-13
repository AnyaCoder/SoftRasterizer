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
    Texture aoTexture;
    Texture specularTexture;
    Texture glossTexture;
    int shininess = 1 << 6;

    std::shared_ptr<Shader> shader;
    Material(std::shared_ptr<Shader> sh = nullptr) : shader(sh) {}
    
    bool loadDiffuseTexture(const std::string& filename) {
        return diffuseTexture.loadFromTGA(filename);
    }

    bool loadNormalTexture(const std::string& filename) {
        return normalTexture.loadFromTGA(filename);
    }

    bool loadAoTexture(const std::string& filename) {
        return aoTexture.loadFromTGA(filename);
    }

    bool loadSpecularTexture(const std::string& filename) {
        return specularTexture.loadFromTGA(filename);
    }

    bool loadGlossTexture(const std::string& filename) {
        return glossTexture.loadFromTGA(filename);
    }
    
};