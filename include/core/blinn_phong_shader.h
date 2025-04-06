// blinn_phong_shader.h
#pragma once

#include "shader.h"

class BlinnPhongShader : public Shader {
public:
    Varyings vertex(const VertexInput& input) override;
    bool fragment(const Varyings& input, Vector3<float>& outColor) override;
};