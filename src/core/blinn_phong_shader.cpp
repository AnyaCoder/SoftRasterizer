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
    vec4f modelNormal4(input.normal, 0.0f); // 0.0f w for direction
    vec4f modelTangent4(input.tangent, 0.0f);
    vec4f modelBitangent4(input.bitangent, 0.0f);

    // Calculate world position
    vec4f worldPos4 = uniform_ModelMatrix * modelPos4;
    output.worldPosition = worldPos4.xyz();

    // Transform normal to world space using Normal Matrix
    output.normal    = (uniform_NormalMatrix * modelNormal4).xyz().normalized();
    output.tangent   = (uniform_NormalMatrix * modelTangent4).xyz().normalized();
    output.bitangent = (uniform_NormalMatrix * modelBitangent4).xyz().normalized();

    // Pass UV coordinates
    output.uv = input.uv;

    // Calculate clip space position
    output.clipPosition = uniform_MVP * modelPos4;

    return output;
}

bool BlinnPhongShader::fragment(const Varyings& input, vec3f& outColor) {
    // --- Determine Normal ---
    vec3f N;
    if (!uniform_NormalTexture.empty()) {
        // Sample normal map (returns color in [0, 1] range)
        vec3f tangentNormalSample = uniform_NormalTexture.sample(input.uv.x, input.uv.y);

        // Map color [0, 1] to normal vector [-1, 1]
        vec3f tangentNormal = (tangentNormalSample * 2.0f) - vec3f(1.0f, 1.0f, 1.0f);
        tangentNormal = tangentNormal.normalized(); // Ensure it's a unit vector

        // Get interpolated TBN basis vectors (renormalize after interpolation)
        vec3f T = input.tangent.normalized();
        vec3f B = input.bitangent.normalized();
        vec3f N_geom = input.normal.normalized(); // Interpolated geometric normal

        // Optional: Re-orthogonalize TBN frame here if interpolation artifacts are visible
        // T = (T - N_geom * N_geom.dot(T)).normalized();
        // B = N_geom.cross(T); // Can recalculate B

        // Transform normal from tangent space to world space
        // N = TBN * tangentNormal (matrix multiplication)
        // Or manually:
        N = T * tangentNormal.x + B * tangentNormal.y + N_geom * tangentNormal.z;
        N = N.normalized(); // Final world-space normal for lighting

    } else {
        // Use interpolated geometric normal if no normal map
        N = input.normal.normalized();
    }

    // --- Lighting Calculation ---
    vec3f V = (uniform_CameraPosition - input.worldPosition).normalized(); // View direction

    // Material properties for this fragment
    vec3f matDiffuse = uniform_DiffuseColor;
    if (!uniform_DiffuseTexture.empty()) {
        matDiffuse = matDiffuse * uniform_DiffuseTexture.sample(input.uv.x, input.uv.y);
    }

    // Start with global ambient term
    vec3f totalColor = uniform_AmbientLight * uniform_AmbientColor; // Use material's ambient property

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
        float specFactor = fastPow(std::max(0.0f, N.dot(H)), uniform_Shininess);
        vec3f specular = uniform_SpecularColor * lightCol * specFactor * attenuation;

        // Add to total color
        totalColor = totalColor + diffuse + specular;
    }

    // Clamp final color
    outColor.x = std::min(1.0f, std::max(0.0f, totalColor.x));
    outColor.y = std::min(1.0f, std::max(0.0f, totalColor.y));
    outColor.z = std::min(1.0f, std::max(0.0f, totalColor.z));

    return true; // Indicate pixel should be written
}