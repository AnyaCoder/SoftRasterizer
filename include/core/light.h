// include/core/light.h
#pragma once

#include "math/vector.h"

enum class LightType {
    DIRECTIONAL,
    POINT
    // Can add SPOT later
};

struct Light {
    LightType type;
    vec3f color = {1.0f, 1.0f, 1.0f};
    float intensity = 1.0f;
    // Common properties

    // For specific types
    vec3f direction = {0.0f, 0.0f, -1.0f}; // For Directional
    vec3f position = {0.0f, 0.0f, 0.0f};   // For Point
    // Add attenuation factors for Point lights if needed (constant, linear, quadratic)

    // Default constructor (e.g., directional)
    Light(LightType t = LightType::DIRECTIONAL) : type(t) {}
};