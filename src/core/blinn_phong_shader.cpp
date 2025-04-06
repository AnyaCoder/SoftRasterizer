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
    output.worldPosition = Vector3<float>(worldPos4.x, worldPos4.y, worldPos4.z);

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

bool BlinnPhongShader::fragment(const Varyings& input, Vector3<float>& outColor) {
    Vector3<float> N = input.worldNormal.normalized(); // Should already be normalized, but safety first
    Vector3<float> V = (uniform_CameraPosition - input.worldPosition).normalized(); // View direction

    // Material properties for this fragment
    Vector3<float> matDiffuse = uniform_Material.diffuseColor;
    if (uniform_Material.hasDiffuseTexture()) {
        matDiffuse = matDiffuse * uniform_Material.diffuseTexture.sample(input.uv.x, input.uv.y);
    }
    Vector3<float> matSpecular = uniform_Material.specularColor;
    // Add specular texture sampling if implemented
    int matShininess = uniform_Material.shininess;

    // Start with global ambient term
    Vector3<float> totalColor = uniform_AmbientLight * uniform_Material.ambientColor; // Use material's ambient property

    // Accumulate light contributions
    for (const auto& light : uniform_Lights) {
        Vector3<float> L;       // Light direction
        Vector3<float> lightCol = light.color * light.intensity;
        float attenuation = 1.0f; // For point lights

        if (light.type == LightType::DIRECTIONAL) {
            L = -light.direction.normalized(); // Direction TO the light
        } else if (light.type == LightType::POINT) {
            Vector3<float> lightVec = light.position - input.worldPosition;
            float dist = lightVec.length();
            L = lightVec.normalized();
            // Add attenuation calculation here based on distance 'dist' if needed
            // attenuation = 1.0f / (constant + linear * dist + quadratic * dist * dist);
        } else {
            continue; // Skip unknown light types
        }

        // Diffuse (Lambertian)
        float diffFactor = std::max(0.0f, N.dot(L));
        Vector3<float> diffuse = matDiffuse * lightCol * diffFactor * attenuation;

        // Specular (Blinn-Phong)
        Vector3<float> H = (L + V).normalized(); // Halfway vector
        float specFactor = fastPow(std::max(0.0f, N.dot(H)), matShininess);
        Vector3<float> specular = matSpecular * lightCol * specFactor * attenuation;

        // Add to total color
        totalColor = totalColor + diffuse + specular;
    }

    // Clamp final color
    outColor.x = std::min(1.0f, std::max(0.0f, totalColor.x));
    outColor.y = std::min(1.0f, std::max(0.0f, totalColor.y));
    outColor.z = std::min(1.0f, std::max(0.0f, totalColor.z));

    return true; // Indicate pixel should be written
}