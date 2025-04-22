// include/core/resource_manager.h
#pragma once
#include <memory>
#include <string>
#include <map>
#include <iostream>
#include "core/model.h"
#include "core/shader.h"
#include "core/texture/texture.h"
// Potentially include shader headers if managing them too

class Model;
class Texture;
class Shader;

class ResourceManager {
public:
    ResourceManager() { std::cout << "ResourceManager" << std::endl; }
    // Use shared_ptr to manage resource lifetime
    std::shared_ptr<Model> loadModel(const std::string& filename);
    std::shared_ptr<Texture> loadTexture(const std::string& filename);
    std::shared_ptr<Shader> loadShader(const std::string& name);

    void clearUnused(); // Optional: for cleanup

private:
    // Caches to avoid reloading
    std::map<std::string, std::shared_ptr<Model>> modelCache;
    std::map<std::string, std::shared_ptr<Texture>> textureCache;
    std::map<std::string, std::shared_ptr<Shader>> shaderCache;

    // Internal helper to load specific texture types (TGA, DDS)
    bool loadObjFromFile(const std::string& filename, Model& model);
};