// src/core/scene.cpp
#include "core/scene.h"
#include "core/resource_manager.h" 
#include "core/blinn_phong_shader.h"
#include <yaml-cpp/yaml.h>
#include <iostream>
#include "io/debug.h"

Scene::Scene(int width, int height, ResourceManager& resManager)
    : resourceManager(resManager), camera({0.0f, 0.0f, 5.0f}, -90.0f, 0.0f) {
    std::cout << "Scene::Scene" << std::endl; 
    camera.setPerspective(45.0f, static_cast<float>(width) / static_cast<float>(height), 0.1f, 100.0f);
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
            vec3f position = {0.0f, 0.0f, 5.0f}; // Default position
            float yaw = -90.0f; // Default yaw (looking down -Z)
            float pitch = 0.0f;   // Default pitch

            if (cameraNode["position"]) {
                position = cameraNode["position"].as<std::vector<float>>();
            } 
            if (cameraNode["yaw"]) {
                yaw = cameraNode["yaw"].as<float>();
            } 
            if (cameraNode["pitch"]) {
                pitch = cameraNode["pitch"].as<float>();
            } 

            camera.setPosition(position);
            camera.setPitchYaw(pitch, yaw);

            float fov = 60.0f, aspect = 1.0f, near = 0.1f, far = 100.0f;
            if (cameraNode["fov"]) fov = cameraNode["fov"].as<float>();
            if (cameraNode["width"] && cameraNode["height"] && cameraNode["height"].as<float>() != 0) {
                 aspect = cameraNode["width"].as<float>() / cameraNode["height"].as<float>();
            } else if (cameraNode["aspect"]) {
                 aspect = cameraNode["aspect"].as<float>();
            }
            if (cameraNode["near"]) near = cameraNode["near"].as<float>();
            if (cameraNode["far"]) far = cameraNode["far"].as<float>();

            camera.setPerspective(fov, aspect, near, far);
            
        } else {
            Debug::LogWarning("Warning: 'camera' node not found in scene file. Using default camera settings");
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
                    Debug::LogWarning("Warning: Unknown light type '{}' in scene file.", type);
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
        } else {
            Debug::LogWarning("Warning: 'light' node not found in scene file.");
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
                        } else {;
                            Debug::LogWarning("Warning: Unknown animation type '{}' in scene file.", animType);
                        }
                    }
                }

                objects.push_back(std::move(obj)); // Move object into vector
            }
        } else {
            Debug::LogWarning("Warning: 'objects' node not found in scene file.");
        }

        return true;
    } catch (const YAML::BadFile& e) {
        Debug::LogError("Error: Could not open or read YAML file '{}'.", filename);
        return false;
    } catch (const YAML::ParserException& e) {
        Debug::LogError("Error parsing YAML file '{}':{}.", filename, e.what());
        return false;
    } catch (const YAML::BadConversion& e) {
        Debug::LogError("Error converting YAML node in '{}':{}.", filename, e.what());
        return false;
    } catch (const std::exception& e) {
        // Catch other potential exceptions (e.g., from resource loading if they throw)
        Debug::LogError("An unexpected error occurred while loading scene '{}':{}.", filename, e.what());
        return false;
   }
}
