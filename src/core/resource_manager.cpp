// src/core/resource_manager.cpp
#include "core/resource_manager.h"
#include "core/texture/tga_texture.h"
#include "core/texture/dds_texture.h"
#include "core/model.h"
#include "core/blinn_phong_shader.h" // Example shader include

#include <iostream>
#include <fstream>
#include <sstream>


// --- Texture Loading ---
std::shared_ptr<Texture> ResourceManager::loadTexture(const std::string& filename) {
    // Check cache first
    auto it = textureCache.find(filename);
    if (it != textureCache.end()) {
        // std::cout << "Cache hit for texture: " << filename << std::endl;
        return it->second;
    }

    std::cout << "Loading texture: " << filename << std::endl;

    // Determine texture type based on extension
    std::shared_ptr<Texture> texture = nullptr;
    std::string lowerFilename = filename;
    for (char &c : lowerFilename) { c = tolower(c); } // Convert to lowercase for extension check

    if (lowerFilename.ends_with(".tga")) {
        texture = std::make_shared<TGATexture>();
    } else if (lowerFilename.ends_with(".dds")) {
        texture = std::make_shared<DDSTexture>();
    } else {
        std::cerr << "\033[31m Error: Unsupported texture format for file: " << filename << "\033[0m " << std::endl;
        return nullptr;
    }

    if (texture && texture->load(filename)) {
        std::cout << "\033[32m Successfully loaded texture: " << filename << " (" << texture->width << "x" << texture->height << ") \033[0m" << std::endl;
        textureCache[filename] = texture; // Add to cache on success
        return texture;
    } else {
        std::cerr << "\033[31m Error: Failed to load texture data from file: " << filename << "\033[0m " << std::endl;
        return nullptr;
    }
}


// --- Model Loading ---

bool ResourceManager::loadObjFromFile(const std::string& filename, Model& model) {
    model.vertices.clear();
    model.normals.clear();
    model.uvs.clear();
    model.faces.clear();
    model.tangents.clear();
    model.bitangents.clear();

    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    std::vector<vec3f> temp_vertices;
    std::vector<vec2f> temp_uvs;
    std::vector<vec3f> temp_normals;
    std::vector<Model::Face> temp_faces;

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::istringstream iss(line);
        std::string type;
        iss >> type;

        if (type == "v") {
            vec3f v;
            iss >> v.x >> v.y >> v.z;
            temp_vertices.push_back(std::move(v));
        }
        else if (type == "vt") {
            vec2f vt;
            iss >> vt.x >> vt.y;
            temp_uvs.push_back(std::move(vt));
        }
        else if (type == "vn") {
            vec3f vn;
            iss >> vn.x >> vn.y >> vn.z;
            temp_normals.push_back(std::move(vn));
        }
        else if (type == "f") {
            Model::Face face;
            char sep;
            int v, t, n, i = 0;
            
            while (iss >> v) {
                face.vertIndex[i] = --v;
                
                if (iss.peek() == '/') {
                    iss >> sep;
                    if (iss.peek() != '/') {
                        iss >> t;
                        face.uvIndex[i] = --t;
                    }
                    if (iss.peek() == '/') {
                        iss >> sep >> n;
                        face.normIndex[i] = --n;
                    }
                }
                i++;
            }
            
            temp_faces.push_back(std::move(face));
        }
    }

    file.close();
    model.vertices = std::move(temp_vertices);
    model.normals = std::move(temp_normals) ;
    model.uvs = std::move(temp_uvs);
    model.faces = std::move(temp_faces);

    std::cout << "\033[32m Successfully Loaded OBJ: " << filename << " (Vertices: " << model.numVertices()
        << ", Normals: " << model.numNormals() << ", UVs: " << model.numUVs()
        << ", Faces: " << model.numFaces() << ") \033[0m" << std::endl;

    model.calculateTangents();
    return true;
}

std::shared_ptr<Model> ResourceManager::loadModel(const std::string& filename) {
    // Check cache
    auto it = modelCache.find(filename);
    if (it != modelCache.end()) {
        // std::cout << "Cache hit for model: " << filename << std::endl;
        return it->second;
    }

    std::cout << "Loading model: " << filename << std::endl;

    auto model = std::make_shared<Model>();

    // Use internal loader function
    if (loadObjFromFile(filename, *model)) {
        modelCache[filename] = model; // Add to cache on success
        return model;
    } else {
        std::cerr << "\033[31m Error: Failed to load model data from file: " << filename << "\033[0m" << std::endl;
        // modelCache[filename] = nullptr; // Cache failure
        return nullptr; // Return null on failure
    }
}


// --- Shader Loading (Example) ---

std::shared_ptr<Shader> ResourceManager::loadShader(const std::string& name) {
     // Check cache
    auto it = shaderCache.find(name);
    if (it != shaderCache.end()) {
        // std::cout << "Cache hit for shader: " << name << std::endl;
        return it->second;
    }

    std::cout << "Loading shader: " << name << std::endl;

    std::shared_ptr<Shader> shader = nullptr;

    // Simple example: Create known shaders by name
    if (name == "BlinnPhong") {
        shader = std::make_shared<BlinnPhongShader>();
    }
    // Add cases for other shaders...
    // else if (name == "Unlit") { shader = std::make_shared<UnlitShader>(); }
    // else if (name == "PBR") { shader = std::make_shared<PBRShader>(); }
    else {
        std::cerr << "\033[31m Error: Unknown shader name requested: " << name << "\033[0m " << std::endl;
        // shaderCache[name] = nullptr; // Cache failure
        return nullptr;
    }

    if (shader) {
        shaderCache[name] = shader;
        std::cout << "\033[32m Successfully loaded shader: " << name << "\033[0m " << std::endl;
        return shader;
    } else {
        // Should not happen with current logic, but good practice
        std::cerr << "\033[31m Error: Failed to create shader object for: " << name << "\033[0m " << std::endl;
        // shaderCache[name] = nullptr; // Cache failure
        return nullptr;
    }
}


// --- Optional: Cache Management ---
void ResourceManager::clearUnused() {
    // Implement logic to clear resources with shared_ptr use_count == 1 (only held by cache)
    // Be careful with dependencies (e.g., models might be needed even if not drawn this frame)
    std::cout << "Clearing unused resources..." << std::endl;
    // Example for textures:
    for (auto it = textureCache.begin(); it != textureCache.end(); /* no increment */) {
        if (it->second.use_count() == 1) {
            std::cout << "Unloading unused texture: " << it->first << std::endl;
            it = textureCache.erase(it);
        } else {
            ++it;
        }
    }

    for (auto it = modelCache.begin(); it != modelCache.end(); /* no increment */) {
        if (it->second.use_count() == 1) {
            std::cout << "Unloading unused model: " << it->first << std::endl;
            it = modelCache.erase(it);
        } else {
            ++it;
        }
    }

    for (auto it = shaderCache.begin(); it != shaderCache.end(); /* no increment */) {
        if (it->second.use_count() == 1) {
            std::cout << "Unloading unused shader: " << it->first << std::endl;
            it = shaderCache.erase(it);
        } else {
            ++it;
        }
    }

}