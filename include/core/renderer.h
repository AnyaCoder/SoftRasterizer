// include/core/renderer.h
#pragma once

#include "framebuffer.h"
#include "shader.h"
#include "model.h"
#include "light.h"
#include "camera.h"
#include "math/matrix.h"
#include "math/transform.h"
#include <vector>
#include <memory>


struct ScreenVertex {
    int x, y;
    float z;
    float invW;
    Varyings varyings;
};

class Renderer {
public:
    Renderer(Framebuffer& fb);

    void setLights(const std::vector<Light>& l);
    void setCamera(const Camera& cam); // Store view/projection matrices

    void clear(const vec3f& color);
    void drawModel(const Model& model, const Transform& transform, const Material& material);

private:
    Framebuffer& framebuffer;
    std::vector<Light> lights;
    mat4 viewMatrix;
    mat4 projectionMatrix;
    vec3f cameraPosition;

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
};
