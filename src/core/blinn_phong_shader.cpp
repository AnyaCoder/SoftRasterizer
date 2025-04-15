// src/core/blinn_phong_shader.cpp
#include "core/blinn_phong_shader.h"
#include <cmath>
#include <algorithm>

template <typename T>
T fastPow(T base, int n) {
    if (n < 0) {
        return static_cast<T>(1) / fastPow(base, -n);
    }
    T res = static_cast<T>(1);
    while (n) {
        if (n & 1) {
            res = res * base;
        }
        base = base * base;
        n >>= 1;
    }
    return res;
}

Varyings BlinnPhongShader::vertex(const VertexInput& input) {
    Varyings output;
    vec4f modelPos4(input.position, 1.0f); // Assuming vec4f exists or use Vector4<float>
    vec3f modelNormal3(input.normal); // 0.0f w for direction
    vec3f modelTangent3(input.tangent);
    vec3f modelBitangent3(input.bitangent);

    // Calculate world position
    vec4f worldPos4 = uniform_ModelMatrix * modelPos4;
    output.worldPosition = worldPos4.xyz();

    // Transform normal to world space using Normal Matrix
    output.normal    = (uniform_NormalMatrix * modelNormal3).normalized();
    output.tangent   = (uniform_NormalMatrix * modelTangent3).normalized();
    output.bitangent = (uniform_NormalMatrix * modelBitangent3).normalized();

    // Pass UV coordinates
    output.uv = input.uv;

    // Calculate clip space position
    output.clipPosition = uniform_MVP * modelPos4;

    return output;
}

bool BlinnPhongShader::fragment(const Varyings& input, vec3f& outColor) {
    // --- Determine Normal ---
    vec3f N;
    if (uniform_UseNormalMap) {
        // Sample normal map (returns color in [0, 1] range)
        vec3f tangentNormalSample = uniform_NormalTexture.sample(input.uv.x, input.uv.y);

        // Map color [0, 1] to normal vector [-1, 1]
        vec3f tangentNormal = (tangentNormalSample * 2.0f) - vec3f(1.0f, 1.0f, 1.0f);
        tangentNormal = tangentNormal.normalized(); // Ensure it's a unit vector

        // Get interpolated TBN basis vectors (renormalize after interpolation)
        vec3f T = input.tangent.normalized();
        vec3f B = input.bitangent.normalized();
        vec3f N_geom = input.normal.normalized(); // Interpolated geometric normal

        N = T * tangentNormal.x + B * tangentNormal.y + N_geom * tangentNormal.z;
        N = N.normalized(); // Final world-space normal for lighting

    } else {
        // Use interpolated geometric normal if no normal map
        N = input.normal.normalized();
    }

    // --- Lighting Calculation ---
    vec3f V = (uniform_CameraPosition - input.worldPosition).normalized(); // View direction

    // Diffuse Color (with texture modulation)
    vec3f matDiffuse = uniform_DiffuseColor;
    if (uniform_UseDiffuseMap) {
        matDiffuse = matDiffuse * uniform_DiffuseTexture.sample(input.uv.x, input.uv.y);
    }

    // Specular Color (with texture override)
    vec3f matSpecular = uniform_SpecularColor;
    if (uniform_UseSpecularMap) {
        matSpecular = uniform_SpecularTexture.sample(input.uv.x, input.uv.y); // Use map value
    }

    // Shininess/Gloss (with texture override)
    int currentShininess = uniform_Shininess; // Default
    if (uniform_UseGlossMap) {
        // Sample gloss map (assume single channel, e.g., .x)
        float glossFactor = uniform_GlossTexture.sample(input.uv.x, input.uv.y).x;
        glossFactor = std::max(0.0f, std::min(1.0f, glossFactor)); // Clamp [0, 1]

        // Map gloss [0, 1] to shininess range [min, max]
        // Adjust min/max range as needed for visual results
        const int minShininess = 2;
        const int maxShininess = 256;
        currentShininess = minShininess + static_cast<int>(
                static_cast<float>(maxShininess - minShininess) * glossFactor
            );
    }

    // Ambient Color (base material property)
    vec3f matAmbient = uniform_AmbientColor;

    // --- Ambient Occlusion ---
    float aoFactor = 1.0f; // Default: no occlusion
    if (uniform_UseAoMap) {
        // Sample AO map (assume single channel, e.g., .x)
        aoFactor = uniform_AoTexture.sample(input.uv.x, input.uv.y).x;
        aoFactor = std::max(0.0f, std::min(1.0f, aoFactor)); // Clamp [0, 1]
    }

    // Calculate final ambient term, modulated by AO
    vec3f ambientTerm = uniform_AmbientLight * matAmbient * aoFactor;
    vec3f totalColor = ambientTerm; // Initialize total color

    // Accumulate light contributions
    for (const auto& light : uniform_Lights) {
        vec3f L;       // Light direction
        vec3f lightCol = light.color * light.intensity;
        float attenuation = 1.0f; // For point lights

        if (light.type == LightType::DIRECTIONAL) {
            L = -light.direction.normalized(); // Direction TO the light
        } else if (light.type == LightType::POINT) {
            vec3f lightVec = light.position - input.worldPosition;
            float dist = lightVec.length();
            L = lightVec.normalized();
            // Example simple distance attenuation (inverse square)
            // attenuation = 1.0f / (1.0f + 0.1f * dist + 0.01f * dist * dist); // Adjust constants as needed
            attenuation = 1.0f / (dist * dist); // Simple inverse square (can be harsh)
            attenuation = std::min(1.0f, std::max(0.0f, attenuation)); // Clamp attenuation
        } else {
            continue; // Skip unknown light types
        }

        // Diffuse (Lambertian)
        float diffFactor = std::max(0.0f, N.dot(L));
        vec3f diffuse = matDiffuse * lightCol * diffFactor * attenuation;

        // Specular (Blinn-Phong)
        vec3f H = (L + V).normalized(); // Halfway vector
        float specFactor = fastPow(std::max(0.0f, N.dot(H)), currentShininess);
        vec3f specular = matSpecular * lightCol * specFactor * attenuation;

        // Add to total color
        totalColor = totalColor + diffuse + specular;
    }

    // Clamp final color
    outColor.x = std::min(1.0f, std::max(0.0f, totalColor.x));
    outColor.y = std::min(1.0f, std::max(0.0f, totalColor.y));
    outColor.z = std::min(1.0f, std::max(0.0f, totalColor.z));

    return true; // Indicate pixel should be written
}