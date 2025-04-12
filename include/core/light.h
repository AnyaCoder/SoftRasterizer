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
    vec3f direction = {0.0f, 0.0f, -1.0f};
    vec3f position = {0.0f, 0.0f, 0.0f};
    Light(LightType t = LightType::DIRECTIONAL) : type(t) {}
};