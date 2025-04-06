#pragma once

#include "math/vector.h"
#include "texture.h" // Assuming you have a Texture class

struct Material {
    vec3f ambientColor = {0.1f, 0.1f, 0.1f};
    vec3f diffuseColor = {0.8f, 0.8f, 0.8f};
    vec3f specularColor = {0.5f, 0.5f, 0.5f};
    int shininess = 32; // Specular exponent (assume integer -> faster)

    Texture diffuseTexture;
    // Texture specularTexture; // Optional: Add specular map support

    bool hasDiffuseTexture() const {
        return !diffuseTexture.empty();
    }
};