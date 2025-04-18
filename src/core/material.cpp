// src/core/material.cpp
#include "core/material.h"

bool Material::loadDiffuseTexture(const std::string& filename) {
    if (filename.ends_with(".tga")) {
        diffuseTexture = std::make_shared<TGATexture>();
    } else if (filename.ends_with(".dds")) {
        diffuseTexture = std::make_shared<DDSTexture>();
    } else {
        return false; // Unsupported format
    }
    return diffuseTexture->load(filename);
}

bool Material::loadNormalTexture(const std::string& filename) {
    if (filename.ends_with(".tga")) {
        normalTexture = std::make_shared<TGATexture>();
    } else if (filename.ends_with(".dds")) {
        normalTexture = std::make_shared<DDSTexture>();
    } else {
        return false;
    }
    return normalTexture->load(filename);
}

bool Material::loadAoTexture(const std::string& filename) {
    if (filename.ends_with(".tga")) {
        aoTexture = std::make_shared<TGATexture>();
    } else if (filename.ends_with(".dds")) {
        aoTexture = std::make_shared<DDSTexture>();
    } else {
        return false;
    }
    return aoTexture->load(filename);
}

bool Material::loadSpecularTexture(const std::string& filename) {
    if (filename.ends_with(".tga")) {
        specularTexture = std::make_shared<TGATexture>();
    } else if (filename.ends_with(".dds")) {
        specularTexture = std::make_shared<DDSTexture>();
    } else {
        return false;
    }
    return specularTexture->load(filename);
}

bool Material::loadGlossTexture(const std::string& filename) {
    if (filename.ends_with(".tga")) {
        glossTexture = std::make_shared<TGATexture>();
    } else if (filename.ends_with(".dds")) {
        glossTexture = std::make_shared<DDSTexture>();
    } else {
        return false;
    }
    return glossTexture->load(filename);
}