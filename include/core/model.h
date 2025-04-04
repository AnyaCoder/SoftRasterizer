#pragma once
#include <vector>
#include <string>
#include "math/vector.h"
#include "core/texture.h"

class Framebuffer;

class Model {
public:
    std::vector<Vector3<float>> vertices;
    std::vector<Vector3<float>> texCoords;
    std::vector<Vector3<float>> normals;
    std::vector<std::vector<int>> faces; // Stores vertex indices
    std::vector<std::vector<int>> faceTexCoords; // Stores texture coordinate indices
    std::vector<std::vector<int>> faceNormals; // Stores normal indices
    Texture diffuseTexture;

    bool loadFromObj(const std::string& filename);
    bool loadDiffuseTexture(const std::string& filename);
    void renderWireframe(Framebuffer& fb, const Vector3<float>& color);
    void renderSolid(Framebuffer& fb, const Vector3<float>& color, const Vector3<float>& lightDir);
    Vector3<float> calculateFaceNormal(const std::vector<int>& faceIndices) const;
};
