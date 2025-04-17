// include/core/scene.h
#pragma once
#include "core/framebuffer.h"
#include "core/renderer.h"
#include "core/model.h"
#include "core/camera.h"
#include "core/material.h"
#include "core/light.h"
#include "math/transform.h"
#include <vector>
#include <string>
#include <memory>

class ThreadPool;

struct SceneObject {
    Model model;
    Material material;
    Transform transform;
    struct Animation {
        enum class Type { None, RotateY } type = Type::None;
        float speed = 0.0f;
    } animation;
};

class Scene {
public:
    Scene(int width, int height);
    bool loadFromYAML(const std::string& filename);
    void update(float deltaTime);
    void render();
    const Framebuffer& getFramebuffer() { return framebuffer; }
    ThreadPool& getThreadPool() { return threadPool; } 

private:
    Framebuffer framebuffer;
    Renderer renderer;
    Camera camera;
    std::vector<Light> lights;
    std::vector<SceneObject> objects;
    ThreadPool threadPool;

    void initializeDefaultScene(); // Fallback if YAML loading fails
};