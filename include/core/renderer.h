// include/core/renderer.h
#pragma once

#include "core/framebuffer.h"
#include "core/shader.h"
#include "core/model.h"
#include "core/light.h"
#include "core/camera.h"
#include "core/threadpool.h"
#include "math/matrix.h"
#include "math/transform.h"
#include <vector>
#include <memory>

class ThreadPool;

struct ScreenVertex {
    int x, y;
    float z;
    float invW;
    Varyings varyings;
};

struct DrawCommand {
    const Model* model = nullptr;
    const Material* material = nullptr;
    mat4 modelMatrix;
};

class Renderer {
public:
    Renderer(Framebuffer& fb, ThreadPool& tp);

    void setLights(const std::vector<Light>& l);
    void setCameraParams(const mat4& view, const mat4& projection, const vec3f& camPos);
    void clear(const vec3f& color);
    void submit(const DrawCommand& command);
private:
    Framebuffer& framebuffer;
    std::vector<Light> lights;
    mat4 viewMatrix;
    mat4 projMatrix;
    vec3f currentCameraPosition;
    ThreadPool& threadPool;

    void drawLine(int x0, int y0, int x1, int y1, const vec3f& color);
    void drawTriangle(ScreenVertex v0, ScreenVertex v1, ScreenVertex v2, const Material& material);
    void drawScanlines(int yStart, int yEnd, const ScreenVertex& vStartA, const ScreenVertex& vEndA,
        const ScreenVertex& vStartB, const ScreenVertex& vEndB, const Material& material);
   
    template <typename T>
    T perspectiveCorrectInterpolate(float t, const T& startVal, const T& endVal, float startInvW, float endInvW) const {
        float currentInvW = startInvW + (endInvW - startInvW) * t;
        if (std::abs(currentInvW) < 1e-6f) {
            return (startVal + endVal) * 0.5f;
        }

        float currentW = 1.0f / currentInvW;
        T startOverW = startVal * startInvW;
        T endOverW = endVal * endInvW;
        T currentOverW = startOverW + (endOverW - startOverW) * t;
        return currentOverW * currentW;
    }

    Varyings interpolateVaryings(float t, const Varyings& start, const Varyings& end, float startInvW, float endInvW) const;
    void setupShaderUniforms(Shader& shader, const DrawCommand& command);
    void processFace(const Model& model, const Material& material, Shader& shader, int faceIndex);
};
