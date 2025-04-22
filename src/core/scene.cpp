// src/core/scene.cpp
#include "core/scene.h"
#include "core/resource_manager.h" 
#include "core/blinn_phong_shader.h"
#include <yaml-cpp/yaml.h>
#include <iostream>

Scene::Scene(int width, int height, ResourceManager& resManager)
    : resourceManager(resManager), 
      camera(vec3f(0, 0, 0), vec3f(0, 0, -1), vec3f(0, 1, 0)) {
    std::cout << "Scene::Scene" << std::endl; 
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

void Scene::render(Renderer& renderer) {
    for (const auto& obj : objects) {

        DrawCommand command;
        command.model = obj.modelPtr.get();
        command.material = obj.materialPtr.get();
        command.modelMatrix = obj.transform.getTransformMatrix();

        renderer.submit(command);
    }
}


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
                cameraNode["width"].as<float>() / cameraNode["height"].as<float>(),
                cameraNode["near"].as<float>(),
                cameraNode["far"].as<float>()
            );
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
                else {
                    std::cerr << "Warning: Unknown light type '" << type << "' in scene file." << std::endl;
                    continue;
                }

                light.color = lightNode["color"].as<std::vector<float>>();
                light.intensity = lightNode["intensity"].as<float>();
                if (light.type == LightType::DIRECTIONAL) {
                    light.direction = vec3f(lightNode["direction"].as<std::vector<float>>()).normalized();
                } else {
                    light.position = lightNode["position"].as<std::vector<float>>();
                }
                lights.push_back(light);
            }
        }

        // Load objects
        objects.clear();
        auto objectsNode = config["objects"];
        if (objectsNode) {
            for (const auto& objNode : objectsNode) {
                SceneObject obj;

                // Load model using ResourceManager
                std::string modelPath = objNode["model"].as<std::string>();
                obj.modelPtr = resourceManager.loadModel(modelPath); // Use ResourceManage

                // Load material properties and textures using ResourceManager
                auto matNode = objNode["material"];

                if (matNode) {
                    obj.materialPtr = std::make_shared<Material>();

                    if (matNode["shader"])
                        obj.materialPtr->shader = resourceManager.loadShader(matNode["shader"].as<std::string>()); // Example shader loading
                    
                    if (matNode["diffuse_texture"])
                        obj.materialPtr->diffuseTexture = resourceManager.loadTexture(matNode["diffuse_texture"].as<std::string>());
                    
                    if (matNode["normal_texture"])
                        obj.materialPtr->normalTexture = resourceManager.loadTexture(matNode["normal_texture"].as<std::string>());
                    
                    if (matNode["ao_texture"])
                        obj.materialPtr->aoTexture = resourceManager.loadTexture(matNode["ao_texture"].as<std::string>());
                    
                    if (matNode["specular_texture"])    
                        obj.materialPtr->specularTexture = resourceManager.loadTexture(matNode["specular_texture"].as<std::string>());
                    
                    if (matNode["gloss_texture"])
                        obj.materialPtr->glossTexture = resourceManager.loadTexture(matNode["gloss_texture"].as<std::string>());
                    
                    if (matNode["ambientColor"])
                        obj.materialPtr->ambientColor = matNode["ambientColor"].as<std::vector<float>>();

                    if (matNode["diffuseColor"])
                        obj.materialPtr->diffuseColor = matNode["diffuseColor"].as<std::vector<float>>();

                    if (matNode["specularColor"])
                        obj.materialPtr->specularColor = matNode["specularColor"].as<std::vector<float>>();

                    if (matNode["shininess"])
                        obj.materialPtr->shininess = matNode["shininess"].as<int>();
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
                    auto animNode = transformNode["animation"];
                    if (animNode && animNode["type"]) {
                        std::string animType = animNode["type"].as<std::string>();
                        if (animType == "rotate_y") {
                            obj.animation.type = SceneObject::Animation::Type::RotateY;
                            obj.animation.speed = animNode["speed"].as<float>();
                        } else {
                            std::cerr << "\033[33m Warning: Unknown animation type '" << animType << "' in scene file. \033[0m" << std::endl;
                        }
                    }
                }

              
                objects.push_back(std::move(obj)); // Move object into vector
            }
        }

        return true;
    } catch (const YAML::Exception& e) {
        std::cerr << "Error parsing YAML file " << filename << ": " << e.what() << std::endl;
        return false;
    }
}
