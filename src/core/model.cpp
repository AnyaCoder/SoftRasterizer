// src/core/model.cpp
#include "core/model.h"
#include "core/framebuffer.h"
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>


const vec3f& Model::getVertex(int index) const {
    // Add bounds checking if needed
    return vertices[index];
}
const vec3f& Model::getNormal(int index) const {
    if (index >= 0 && index < normals.size()) {
        return normals[index];
    }
    static const Vector3<float> default_normal = {0.0f, 1.0f, 0.0f};
    std::cerr << "Warning: Normal index out of bounds or invalid." << std::endl;
    return default_normal;
}
const vec2f& Model::getUV(int index) const {
    if (index >= 0 && index < uvs.size()) {
        return uvs[index];
    }
    static const vec2f default_uv = {0.0f, 0.0f};
    std::cerr << "Warning: UV index out of bounds or invalid." << std::endl;
    return default_uv;
}
const vec3f& Model::getTangent(int index) const {
    if (index >= 0 && index < tangents.size()) {
        return tangents[index];
    }
    static const vec3f default_tangent = {1.0f, 0.0f, 0.0f};
    std::cerr << "Warning: Tangent index out of bounds or invalid." << std::endl;
    return default_tangent;
}

const vec3f& Model::getBitangent(int index) const {
     if (index >= 0 && index < bitangents.size()) {
        return bitangents[index];
    }
    static const vec3f default_bitangent = {0.0f, 0.0f, 1.0f};
    std::cerr << "Warning: Bitangent index out of bounds or invalid." << std::endl;
    return default_bitangent;
}
const Model::Face& Model::getFace(int index) const {
    return faces[index];
}


// --- Tangent Calculation ---
// Basic implementation - assumes valid UVs and non-degenerate triangles
// More robust implementations exist (e.g., using MikkTSpace)
void Model::calculateTangents() {
    tangents.assign(numVertices(), vec3f(0.0f, 0.0f, 0.0f));
    bitangents.assign(numVertices(), vec3f(0.0f, 0.0f, 0.0f));

    for (size_t i = 0; i < numFaces(); ++i) {
        const Face& face = getFace((int)i);

        // Get vertices and UVs for the face
        const vec3f& v0 = getVertex(face.vertIndex[0]);
        const vec3f& v1 = getVertex(face.vertIndex[1]);
        const vec3f& v2 = getVertex(face.vertIndex[2]);
        const vec2f& uv0 = getUV(face.uvIndex[0]);
        const vec2f& uv1 = getUV(face.uvIndex[1]);
        const vec2f& uv2 = getUV(face.uvIndex[2]);

        // Edges of the triangle & delta UVs
        vec3f edge1 = v1 - v0;
        vec3f edge2 = v2 - v0;
        vec2f deltaUV1 = uv1 - uv0;
        vec2f deltaUV2 = uv2 - uv0;

        float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
        // Handle potential division by zero if UVs are degenerate
        if (std::isinf(f) || std::isnan(f)) {
            f = 0.0f; // Avoid issues, tangent/bitangent will be zero
        }

        vec3f tangent;
        tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
        tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
        tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

        vec3f bitangent;
        bitangent.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
        bitangent.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
        bitangent.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);

        // Accumulate tangents and bitangents for each vertex of the face
        for (int j = 0; j < 3; ++j) {
            tangents[face.vertIndex[j]] += tangent;
            bitangents[face.vertIndex[j]] += bitangent;
        }
    }

    // Orthogonalize and normalize tangents/bitangents for each vertex
    for (size_t i = 0; i < numVertices(); ++i) {
        const vec3f& n = getNormal((int)i);
        vec3f& t = tangents[i];
        vec3f& b = bitangents[i];

        if (t.length() > 0 && n.length() > 0) {
            // Gram-Schmidt orthogonalize: t = normalize(t - n * dot(n, t))
            t = (t - n * n.dot(t)).normalized();

            // Recalculate bitangent B = cross(N, T)
            // Check handedness (important if UVs are mirrored)
            if (n.cross(t).dot(b) < 0.0f) {
                t = t * -1.0f; // Flip tangent to match UV space handedness
            }
            b = n.cross(t).normalized(); // Ensure B is orthogonal and normalized
        } else {
            // If tangent is zero or normal is invalid, create an arbitrary basis
            // This might happen for vertices not used in any face or with bad data
            vec3f up = (std::abs(n.y) < 0.99f) ? vec3f(0.0f, 1.0f, 0.0f) : vec3f(1.0f, 0.0f, 0.0f);
            t = n.cross(up).normalized();
            b = n.cross(t).normalized();
        }


        // Fallback if normalization failed (e.g., zero vectors)
        if (std::isnan(t.x) || std::isinf(t.x)) t = vec3f(1,0,0);
        if (std::isnan(b.x) || std::isinf(b.x)) b = vec3f(0,0,1);

    }

    std::cout << "Calculated tangents and bitangents for " << numVertices() << " vertices." << std::endl;
}