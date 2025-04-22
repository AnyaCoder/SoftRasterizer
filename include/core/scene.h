// include/core/scene.h
#pragma once
#include "core/framebuffer.h"
#include "core/renderer.h"
#include "core/model.h"
#include "core/camera.h"
#include "core/material.h"
#include "core/light.h"
#include "core/resource_manager.h" 
#include "math/transform.h"
#include <vector>
#include <string>
#include <memory>

class ThreadPool;

struct SceneObject {
    std::shared_ptr<Model> modelPtr;
    std::shared_ptr<Material> materialPtr;
    Transform transform;
    struct Animation {
        enum class Type { None, RotateY } type = Type::None;
        float speed = 0.0f;
    } animation;
};

class Scene {
public:
    Scene(int width, int height, ResourceManager& resManager);
    bool loadFromYAML(const std::string& filename);
    void update(float deltaTime);
    void render(Renderer& renderer);

    Camera& getCamera() { return camera; }
    auto& getObjects() { return objects; }
    auto& getLights() { return lights; }
    
private:
    ResourceManager& resourceManager; 
    Camera camera;
    std::vector<Light> lights;
    std::vector<SceneObject> objects;

};