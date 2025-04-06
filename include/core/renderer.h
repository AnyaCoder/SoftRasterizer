// renderer.h
#pragma once

#include "framebuffer.h"
#include "shader.h"
#include "model.h" // Include model to get access to mesh data
#include "light.h"
#include "camera.h"
#include "math/matrix.h"
#include <vector>
#include <memory> // For std::unique_ptr

// Structure to hold vertex data after vertex shader and perspective divide/viewport transform
struct ScreenVertex {
    int x, y;        // Screen coordinates
    float z;         // Depth (typically in NDC or adjusted screen space)
    float invW;      // 1/w for perspective correct interpolation
    Varyings varyings; // Store the full varying data
};


class Renderer {
public:
    Renderer(Framebuffer& fb);

    void setShader(std::shared_ptr<Shader> sh);
    void setLights(const std::vector<Light>& l);
    void setCamera(const Camera& cam); // Store view/projection matrices

    void clear(const Vector3<float>& color);
    void drawModel(Model& model, const Matrix4x4& modelMatrix, const Material& material);

private:
    Framebuffer& framebuffer;
    std::shared_ptr<Shader> currentShader;
    std::vector<Light> lights;
    Matrix4x4 viewMatrix;
    Matrix4x4 projectionMatrix;
    Vector3<float> cameraPosition;

    void drawLine(int x0, int y0, int x1, int y1, const vec3f& color);
    void drawTriangle(ScreenVertex v0, ScreenVertex v1, ScreenVertex v2);
    void drawScanlines(int yStart, int yEnd, const ScreenVertex& vStartA, const ScreenVertex& vEndA,
        const ScreenVertex& vStartB, const ScreenVertex& vEndB);
   
    // Helper for perspective correct interpolation
    template <typename T>
    T perspectiveCorrectInterpolate(float t, const T& startVal, const T& endVal, float startInvW, float endInvW) const {
        float currentInvW = startInvW + (endInvW - startInvW) * t;
        if (std::abs(currentInvW) < 1e-6f) {
            return (startVal + endVal) * 0.5f; // 返回平均值，避免突变
        }

        float currentW = 1.0f / currentInvW;
        T startOverW = startVal * startInvW;
        T endOverW = endVal * endInvW;
        T currentOverW = startOverW + (endOverW - startOverW) * t;
        return currentOverW * currentW;
    }

     // Overload for Varyings struct
    Varyings interpolateVaryings(float t, const Varyings& start, const Varyings& end, float startInvW, float endInvW) const;
};
