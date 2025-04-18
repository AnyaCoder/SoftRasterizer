// include/core/model.h
#pragma once
#include <vector>
#include <string>
#include "math/vector.h"
#include "math/matrix.h"
#include "core/texture/texture.h"

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
    void calculateTangents();

    // Accessors for geometry data
    size_t numVertices() const { return vertices.size(); }
    size_t numFaces() const { return faces.size(); }
    size_t numNormals() const { return normals.size(); }
    size_t numUVs() const { return uvs.size(); }
    size_t numTangents() const { return tangents.size(); }    
    size_t numBitangents() const { return bitangents.size(); }


    const vec3f& getVertex(int index) const;
    const vec3f& getNormal(int index) const;
    const vec2f& getUV(int index) const;
    const Vector3<float>& getTangent(int index) const;  
    const Vector3<float>& getBitangent(int index) const;
    const Face& getFace(int index) const;

    std::vector<vec3f> vertices;
    std::vector<vec3f> normals;
    std::vector<vec2f> uvs;
    std::vector<vec3f> tangents;  
    std::vector<vec3f> bitangents;
    std::vector<Face> faces;

};