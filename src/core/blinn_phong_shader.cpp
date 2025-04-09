// blinn_phong_shader.cpp
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

    // Calculate world position
    vec4f worldPos4 = uniform_ModelMatrix * modelPos4;
    output.worldPosition = vec3f(worldPos4.x, worldPos4.y, worldPos4.z);

    // Transform normal to world space using Normal Matrix
    vec4f modelNormal4(input.normal, 0.0f); // 0.0f w for direction
    vec4f worldNormal4 = uniform_NormalMatrix * modelNormal4;
    output.worldNormal = worldNormal4.xyz().normalized();

    // Pass UV coordinates
    output.uv = input.uv;

    // Calculate clip space position
    output.clipPosition = uniform_MVP * modelPos4;
    return output;
}

bool BlinnPhongShader::fragment(const Varyings& input, vec3f& outColor) {
    vec3f N = input.worldNormal.normalized(); // Should already be normalized, but safety first
    vec3f V = (uniform_CameraPosition - input.worldPosition).normalized(); // View direction

    // Material properties for this fragment
    vec3f matDiffuse = uniform_DiffuseColor;
    if (!uniform_DiffuseTexture.empty()) {
        matDiffuse = matDiffuse * uniform_DiffuseTexture.sample(input.uv.x, input.uv.y);
    }
    vec3f matSpecular = uniform_SpecularColor;
    // Add specular texture sampling if implemented
    int matShininess = uniform_Shininess;

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
            // Add attenuation calculation here based on distance 'dist' if needed
            // attenuation = 1.0f / (constant + linear * dist + quadratic * dist * dist);
        } else {
            continue; // Skip unknown light types
        }

        // Diffuse (Lambertian)
        float diffFactor = std::max(0.0f, N.dot(L));
        vec3f diffuse = matDiffuse * lightCol * diffFactor * attenuation;

        // Specular (Blinn-Phong)
        vec3f H = (L + V).normalized(); // Halfway vector
        float specFactor = fastPow(std::max(0.0f, N.dot(H)), matShininess);
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