// include/core/shader.h
#pragma once

#include "math/vector.h"
#include "math/matrix.h"
#include "core/light.h"
#include "core/texture/texture.h"
#include "core/material.h"
#include <vector>
#include <unordered_map>
#include <string>
#include <memory>

// Data input to the vertex shader (per vertex)
struct VertexInput {
    vec3f position;
    vec3f normal;
    vec2f uv;
    vec3f tangent;  
    vec3f bitangent;
};

// Data output from vertex shader, interpolated for fragment shader
struct Varyings {
    vec4f clipPosition; // Homogeneous coordinates (required!)
    vec3f worldPosition;
    vec3f normal; // Need to transform normal correctly
    vec2f uv;
    vec3f tangent;
    vec3f bitangent;
    // Add other interpolated data as needed
};

class Shader {
public:
    virtual ~Shader() = default;

    // --- Uniforms --- (data constant per draw call) 
    mat4 uniform_ModelMatrix;
    mat4 uniform_ViewMatrix;
    mat4 uniform_ProjectionMatrix;
    mat4 uniform_MVP; // Model * View * Projection
    mat3 uniform_NormalMatrix; // Transpose(Inverse(ModelViewMatrix)) for normals
    
    // --- Material Properties --- (might be part of a Material uniform block later)
    vec3f uniform_AmbientColor;
    vec3f uniform_DiffuseColor;
    vec3f uniform_SpecularColor;
    int uniform_Shininess;

    // Textures
    std::shared_ptr<Texture> uniform_DiffuseTexture;
    bool uniform_UseDiffuseMap = false;

    std::shared_ptr<Texture> uniform_NormalTexture;
    bool uniform_UseNormalMap = false;

    std::shared_ptr<Texture> uniform_AoTexture;
    bool uniform_UseAoMap = false;

    std::shared_ptr<Texture> uniform_SpecularTexture;
    bool uniform_UseSpecularMap = false;

    std::shared_ptr<Texture> uniform_GlossTexture;
    bool uniform_UseGlossMap = false;

    // --- Lighting Uniforms ---
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
    virtual bool fragment(const Varyings& input, vec3f& outColor,
        const vec2f& uv_ddx, const vec2f& uv_ddy) = 0;
};