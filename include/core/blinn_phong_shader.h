// include/core/blinn_phong_shader.h
#pragma once
#include "math/vector.h"
#include "core/shader.h"
#include <iostream>

class BlinnPhongShader : public Shader {
public:
    Varyings vertex(const VertexInput& input) override;
    bool fragment(const Varyings& input, vec3f& outColor,
        const vec2f& uv_ddx, const vec2f& uv_ddy) override;
};