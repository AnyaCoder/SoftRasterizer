#pragma once
#include <vector>
#include <string>
#include "math/vector.h"
#include "math/matrix.h"
#include "core/texture.h"

class Model {
public:
    struct Face {
        int vertIndex[3];
        int uvIndex[3];
        int normIndex[3];

        Face() = default;
    };

    Model() = default;

    bool loadFromObj(const std::string& filename);
    // Texture loading now returns a Texture object or loads into internal storage
    bool loadDiffuseTexture(const std::string& filename); // Keep for loading

    // Accessors for geometry data
    size_t numVertices() const { return vertices.size(); }
    size_t numFaces() const { return faces.size(); }
    size_t numNormals() const { return normals.size(); }
    size_t numUVs() const { return uvs.size(); }

    const Vector3<float>& getVertex(int index) const;
    const Vector3<float>& getNormal(int index) const;
    const Vector2<float>& getUV(int index) const;
    const Face& getFace(int index) const;

    const Texture& getDiffuseTexture() const { return diffuseTexture; } // Getter for texture

    std::vector<Vector3<float>> vertices;
    std::vector<Vector3<float>> normals;
    std::vector<Vector2<float>> uvs;
    std::vector<Face> faces;
    Texture diffuseTexture; // Store the loaded texture

    // bool parseFace(const std::string& line, Face& face); // Example helper
};