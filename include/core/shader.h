// include/core/shader.h
#pragma once

#include "math/vector.h"
#include "math/matrix.h"
#include "light.h"
#include "material.h"
#include <vector>
#include <map>
#include <string>
#include <memory>

// Data input to the vertex shader (per vertex)
struct VertexInput {
    vec3f position;
    vec3f normal;
    vec2f uv;
    // Add tangent/bitangent if doing normal mapping
};

// Data output from vertex shader, interpolated for fragment shader
struct Varyings {
    vec4f clipPosition; // Homogeneous coordinates (required!)
    vec3f worldPosition;
    vec3f worldNormal; // Need to transform normal correctly
    vec2f uv;
    Material material;
    // Add other interpolated data as needed
};

class Shader {
public:
    virtual ~Shader() = default;

    // Uniforms (data constant per draw call)
    // Using specific setters is often cleaner initially than std::any
    Matrix4x4 uniform_ModelMatrix;
    Matrix4x4 uniform_ViewMatrix;
    Matrix4x4 uniform_ProjectionMatrix;
    Matrix4x4 uniform_MVP; // Model * View * Projection
    Matrix4x4 uniform_NormalMatrix; // Transpose(Inverse(ModelViewMatrix)) for normals
    vec3f uniform_AmbientColor;
    vec3f uniform_DiffuseColor;
    vec3f uniform_SpecularColor;
    int uniform_Shininess;
    Texture uniform_DiffuseTexture;
    vec3f uniform_CameraPosition;
    std::vector<Light> uniform_Lights;

    vec3f uniform_AmbientLight = {0.1f, 0.1f, 0.1f}; // Global ambient term

    // --- Shader Stages ---

    // Processes a single vertex
    // Input: vertex attributes
    // Output: Varyings struct (including clip-space position)
    virtual Varyings vertex(const VertexInput& input) = 0;

    // Processes a single fragment
    // Input: Interpolated varyings
    // Output: Final color (or discard) + boolean indicating if pixel should be written
    virtual bool fragment(const Varyings& input, vec3f& outColor) = 0;
};