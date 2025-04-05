#pragma once
#include <vector>
#include <string>
#include "math/vector.h"
#include "math/matrix.h"
#include "core/texture.h"

class Framebuffer;

class Model {
public:
    std::vector<vec3f> vertices;
    std::vector<vec3f> texCoords;
    std::vector<vec3f> normals;
    std::vector<std::vector<int>> faces; // Stores vertex indices
    std::vector<std::vector<int>> faceTexCoords; // Stores texture coordinate indices
    std::vector<std::vector<int>> faceNormals; // Stores normal indices
    Texture diffuseTexture;

    bool loadFromObj(const std::string& filename);
    bool loadDiffuseTexture(const std::string& filename);
    void renderWireframe(Framebuffer& fb, const vec3f& color);
    void renderSolid(Framebuffer& fb, float near, float far, const Matrix4x4& mvp, const vec3f& color, const vec3f& lightDir);
    vec3f calculateFaceNormal(const std::vector<int>& faceIndices) const;
};
