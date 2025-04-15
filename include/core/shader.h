// include/core/shader.h
#pragma once

#include "math/vector.h"
#include "math/matrix.h"
#include "core/light.h"
#include "core/texture.h"
#include "core/material.h"
#include <vector>
#include <map>
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
    Texture uniform_DiffuseTexture;
    bool uniform_UseDiffuseMap = false; // Optional: Add flags for all maps for clarity

    Texture uniform_NormalTexture;
    bool uniform_UseNormalMap = false;

    Texture uniform_AoTexture;          // Added AO Texture uniform
    bool uniform_UseAoMap = false;      // Added AO flag

    Texture uniform_SpecularTexture;    // Added Specular Texture uniform
    bool uniform_UseSpecularMap = false;// Added Specular flag

    Texture uniform_GlossTexture;       // Added Gloss Texture uniform
    bool uniform_UseGlossMap = false;   // Added Gloss flag

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
    virtual bool fragment(const Varyings& input, vec3f& outColor) = 0;
};