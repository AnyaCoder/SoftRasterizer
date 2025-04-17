// src/core/scene.cpp
#include "core/scene.h"
#include "core/blinn_phong_shader.h"
#include <yaml-cpp/yaml.h>
#include <iostream>

Scene::Scene(int width, int height)
    : threadPool(std::max(1U, std::thread::hardware_concurrency() - 1)),
      framebuffer(width, height, threadPool),
      renderer(framebuffer, threadPool), 
      camera(vec3f(0, 0, 0), vec3f(0, 0, -1), vec3f(0, 1, 0)) {}

bool Scene::loadFromYAML(const std::string& filename) {
    try {
        YAML::Node config = YAML::LoadFile(filename);

        // Load camera
        auto cameraNode = config["camera"];
        if (cameraNode) {
            vec3f position = cameraNode["position"].as<std::vector<float>>();
            vec3f target = cameraNode["target"].as<std::vector<float>>();
            vec3f up = cameraNode["up"].as<std::vector<float>>();
            camera = Camera(position, target, up);
            camera.setPerspective(
                cameraNode["fov"].as<float>(),
                static_cast<float>(framebuffer.getWidth()) / framebuffer.getHeight(),
                cameraNode["near"].as<float>(),
                cameraNode["far"].as<float>()
            );
            renderer.setCamera(camera);
        }

        // Load lights
        lights.clear();
        auto lightsNode = config["lights"];
        if (lightsNode) {
            for (const auto& lightNode : lightsNode) {
                Light light;
                std::string type = lightNode["type"].as<std::string>();
                if (type == "directional") light.type = LightType::DIRECTIONAL;
                else if (type == "point") light.type = LightType::POINT;
                else continue;

                light.color = lightNode["color"].as<std::vector<float>>();
                light.intensity = lightNode["intensity"].as<float>();
                if (light.type == LightType::DIRECTIONAL) {
                    light.direction = vec3f(lightNode["direction"].as<std::vector<float>>()).normalized();
                } else {
                    light.position = lightNode["position"].as<std::vector<float>>();
                }
                lights.push_back(light);
            }
            renderer.setLights(lights);
        }

        // Load objects
        objects.clear();
        auto objectsNode = config["objects"];
        if (objectsNode) {
            for (const auto& objNode : objectsNode) {
                SceneObject obj;
                // Load model
                std::string modelPath = objNode["model"].as<std::string>();
                if (!obj.model.loadFromObj(modelPath)) {
                    std::cerr << "Failed to load model: " << modelPath << std::endl;
                    continue;
                }

                // Load material
                auto matNode = objNode["material"];
                obj.material = Material(std::make_shared<BlinnPhongShader>());
                if (matNode["diffuse_texture"]) {
                    obj.material.loadDiffuseTexture(matNode["diffuse_texture"].as<std::string>());
                }
                if (matNode["normal_texture"]) {
                    obj.material.loadNormalTexture(matNode["normal_texture"].as<std::string>());
                }
                if (matNode["specular_texture"]) {
                    obj.material.loadSpecularTexture(matNode["specular_texture"].as<std::string>());
                }

                // Load transform
                auto transformNode = objNode["transform"];
                if (transformNode) {
                    if (transformNode["position"]) {
                        obj.transform.setPosition(transformNode["position"].as<std::vector<float>>());
                    }
                    if (transformNode["rotation"]) {
                        obj.transform.setRotationEulerZYX(transformNode["rotation"].as<std::vector<float>>());
                    }
                    if (transformNode["scale"]) {
                        obj.transform.setScale(transformNode["scale"].as<std::vector<float>>());
                    }
                }

                // Load animation
                auto animNode = transformNode["animation"];
                if (animNode && animNode["type"]) {
                    std::string animType = animNode["type"].as<std::string>();
                    if (animType == "rotate_y") {
                        obj.animation.type = SceneObject::Animation::Type::RotateY;
                        obj.animation.speed = animNode["speed"].as<float>();
                    }
                }

                auto& mat = obj.material;
                auto& shader = *mat.shader;

                // Lighting
                shader.uniform_Lights = lights;
                shader.uniform_AmbientLight = {0.1f, 0.1f, 0.1f}; // Or set globally elsewhere

                // Base Material Properties
                shader.uniform_AmbientColor = mat.ambientColor;
                shader.uniform_DiffuseColor = mat.diffuseColor;
                shader.uniform_SpecularColor = mat.specularColor; // Base value
                shader.uniform_Shininess = mat.shininess;         // Base value

                // Texture Uniforms and Flags
                shader.uniform_DiffuseTexture = mat.diffuseTexture;
                shader.uniform_UseDiffuseMap = !mat.diffuseTexture.empty(); // Set flag

                shader.uniform_NormalTexture = mat.normalTexture;
                shader.uniform_UseNormalMap = !mat.normalTexture.empty();

                shader.uniform_AoTexture = mat.aoTexture; // Set AO map
                shader.uniform_UseAoMap = !mat.aoTexture.empty();

                shader.uniform_SpecularTexture = mat.specularTexture; // Set Specular map
                shader.uniform_UseSpecularMap = !mat.specularTexture.empty();

                shader.uniform_GlossTexture = mat.glossTexture; // Set Gloss map
                shader.uniform_UseGlossMap = !mat.glossTexture.empty();

                objects.push_back(obj);
            }
        }

        return true;
    } catch (const YAML::Exception& e) {
        std::cerr << "Error parsing YAML file " << filename << ": " << e.what() << std::endl;
        initializeDefaultScene();
        return false;
    }
}

void Scene::initializeDefaultScene() {
    // Fallback: Setup a basic scene
    camera = Camera(vec3f(0, 1, 3), vec3f(0, 0, 0), vec3f(0, 1, 0));
    camera.setPerspective(45.0f, static_cast<float>(framebuffer.getWidth()) / framebuffer.getHeight(), 0.1f, 100.0f);
    renderer.setCamera(camera);

    Light dirLight(LightType::DIRECTIONAL);
    dirLight.direction = vec3f(0.707f, 0.0f, -0.707f).normalized();
    dirLight.color = vec3f(1.0f, 1.0f, 1.0f);
    dirLight.intensity = 1.0f;
    lights = {dirLight};
    renderer.setLights(lights);
}

void Scene::update(float deltaTime) {
    for (auto& obj : objects) {
        if (obj.animation.type == SceneObject::Animation::Type::RotateY) {
            float angle = obj.transform.getRotationEulerZYX().y + obj.animation.speed * deltaTime;
            if (angle > 360.0f) angle -= 360.0f;
            obj.transform.setRotationEulerZYX({0.0f, angle, 0.0f});
        }
    }
}

void Scene::render() {
    renderer.clear(vec3f(0.5f, 0.5f, 0.5f));
    for (const auto& obj : objects) {
        renderer.drawModel(obj.model, obj.transform, obj.material);
    }
    framebuffer.flipVertical();
}