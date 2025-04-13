// include/core/blinn_phong_shader.h
#pragma once
#include "math/vector.h"
#include "core/shader.h"

class BlinnPhongShader : public Shader {
public:
    Varyings vertex(const VertexInput& input) override;
    bool fragment(const Varyings& input, vec3f& outColor) override;
};