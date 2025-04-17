// src/core/renderer.cpp
#include "core/renderer.h"
#include "core/camera.h"
#include "core/threadpool.h"

Renderer::Renderer(Framebuffer& fb, ThreadPool& tp) 
    : framebuffer(fb), 
      threadPool(tp) {}

void Renderer::setLights(const std::vector<Light>& l) {
    lights = l;
}

void Renderer::setCamera(const Camera& cam) {
    viewMatrix = cam.getViewMatrix();
    projectionMatrix = cam.getProjectionMatrix();
    cameraPosition = cam.getPosition(); // Need camera world position for lighting
}

void Renderer::clear(const vec3f& color) {
    framebuffer.clear(color);
    framebuffer.clearZBuffer();
}

// Interpolate the whole Varyings struct perspective-correctly
Varyings Renderer::interpolateVaryings(float t, const Varyings& start, const Varyings& end, float startInvW, float endInvW) const {
    Varyings result;
    float currentInvW = startInvW + (endInvW - startInvW) * t;
    if (std::abs(currentInvW) < 1e-6f) return start; // Or handle differently

    float currentW = 1.0f / currentInvW;

    // Interpolate each component perspective-correctly
    // Note: clipPosition is usually not needed in fragment shader, but other varyings are.
    result.worldPosition = perspectiveCorrectInterpolate(t, start.worldPosition, end.worldPosition, startInvW, endInvW);
    result.uv = perspectiveCorrectInterpolate(t, start.uv, end.uv, startInvW, endInvW);
    // Interpolate TBN vectors (will be normalized in fragment shader)
    result.normal = perspectiveCorrectInterpolate(t, start.normal, end.normal, startInvW, endInvW);
    result.tangent = perspectiveCorrectInterpolate(t, start.tangent, end.tangent, startInvW, endInvW);
    result.bitangent = perspectiveCorrectInterpolate(t, start.bitangent, end.bitangent, startInvW, endInvW);

    return result;
}


void Renderer::drawModel(const Model& model, const Transform& transform, const Material& material) {
    if (!material.shader) {
        std::cerr << "Error: No shader set for rendering!" << std::endl;
        return;
    }

    auto& shader = *material.shader; // Use reference for convenience

    // Matrices
    mat4 modelMatrix = transform.getTransformMatrix();
    mat3 normalMatrix = transform.getNormalMatrix(); 
    shader.uniform_ModelMatrix = modelMatrix;
    shader.uniform_ViewMatrix = viewMatrix;
    shader.uniform_ProjectionMatrix = projectionMatrix;
    shader.uniform_MVP = projectionMatrix * viewMatrix * modelMatrix;
    shader.uniform_NormalMatrix = normalMatrix;

    // Lighting
    shader.uniform_CameraPosition = cameraPosition;

    int numFaces = static_cast<int>(model.numFaces());
#ifdef MultiThreading
    int maxThreads = threadPool.getNumThreads();
    int numThreads = std::max(1, std::min(maxThreads, numFaces));
    int facesPerThread = (numFaces + numThreads - 1) / numThreads; // Ceiling division
    // --- Vertex Processing & Triangle Assembly ---
#else
    int facesPerThread = 1; 
#endif
    for (int startFace = 0; startFace < numFaces; startFace += facesPerThread) {

        int endFace = std::min(startFace + facesPerThread, numFaces);
#ifdef MultiThreading
        threadPool.enqueue([this, &model, &material, startFace, endFace]() {
#endif
            for (int i = startFace; i < endFace; ++i) {
                Model::Face face = model.getFace(i);
                ScreenVertex screenVertices[3];
                Varyings varyings[3];
                bool triangleVisible = false;

                // Vertex processing
                for (int j = 0; j < 3; ++j) {
                    VertexInput vInput;
                    vInput.position = model.getVertex(face.vertIndex[j]);
                    vInput.normal = model.getNormal(face.normIndex[j]);
                    vInput.uv = model.getUV(face.uvIndex[j]);
                    vInput.tangent = model.getTangent(face.vertIndex[j]);
                    vInput.bitangent = model.getBitangent(face.vertIndex[j]);

                    varyings[j] = material.shader->vertex(vInput);

                    // Check if vertex is in front of near plane and valid
                    float w = varyings[j].clipPosition.w;
                    float z = varyings[j].clipPosition.z;
                    if (w > 0 && z >= 0) {
                        triangleVisible = true;
                    }
                }

                if (!triangleVisible) continue;

                // Perspective division and viewport transform
                for (int j = 0; j < 3; ++j) {
                    float w = varyings[j].clipPosition.w;
                    if (w <= 0) continue;  // Skip invalid vertices
                    float invW = 1.0f / w;
                    vec3f ndcPos = {
                        varyings[j].clipPosition.x * invW,
                        varyings[j].clipPosition.y * invW,
                        varyings[j].clipPosition.z * invW
                    };

                    screenVertices[j].x = static_cast<int>((ndcPos.x + 1.0f) * 0.5f * framebuffer.getWidth());
                    screenVertices[j].y = static_cast<int>((ndcPos.y + 1.0f) * 0.5f * framebuffer.getHeight());
                    screenVertices[j].z = (ndcPos.z + 1.0f) * 0.5f;
                    screenVertices[j].invW = invW;
                    screenVertices[j].varyings = varyings[j];
                }

                // Backface culling
                vec2f p0 = {static_cast<float>(screenVertices[0].x), static_cast<float>(screenVertices[0].y)};
                vec2f p1 = {static_cast<float>(screenVertices[1].x), static_cast<float>(screenVertices[1].y)};
                vec2f p2 = {static_cast<float>(screenVertices[2].x), static_cast<float>(screenVertices[2].y)};
                float signedArea = (p1.x - p0.x) * (p2.y - p0.y) - (p2.x - p0.x) * (p1.y - p0.y);
                if (signedArea < 0) continue;

                drawTriangle(screenVertices[0], screenVertices[1], screenVertices[2], material);
            }
#ifdef MultiThreading
        });
#endif
    }

#ifdef MultiThreading
    // Wait for all tasks to complete
    threadPool.waitForCompletion();
#endif
}


// --- Rasterization (Modified Framebuffer logic moved/adapted here) ---

void Renderer::drawLine(int x0, int y0, int x1, int y1, const vec3f& color) {

    bool steep = abs(y1 - y0) > abs(x1 - x0);
    if (steep) {
        std::swap(x0, y0);
        std::swap(x1, y1);
    }
    if (x0 > x1) {
        std::swap(x0, x1);
        std::swap(y0, y1);
    }

    int dx = x1 - x0;
    int dy = abs(y1 - y0);
    int err = dx / 2;
    int ystep = (y0 < y1) ? 1 : -1;
    int y = y0;

    for (int x = x0; x <= x1; x++) {
        if (steep) {
            framebuffer.setPixel(y, x, color);
        } else {
            framebuffer.setPixel(x, y, color);
        }
        err -= dy;
        if (err < 0) {
            y += ystep;
            err += dx;
        }
    }
}

void Renderer::drawTriangle(ScreenVertex v0, ScreenVertex v1, ScreenVertex v2, const Material& material) {
    // Sort vertices by y-coordinate (v0.y <= v1.y <= v2.y)
    if (v0.y > v1.y) { std::swap(v0, v1); }
    if (v0.y > v2.y) { std::swap(v0, v2); }
    if (v1.y > v2.y) { std::swap(v1, v2); }

    // Handle degenerate triangles (horizontal line)
    if (v0.y == v2.y || (v0.x == v1.x && v1.x == v2.x)) return;

    // Draw top part (v0.y to v1.y) - Flat bottom triangle
    if (v0.y < v1.y) {
        drawScanlines(v0.y, v1.y, v0, v2, v0, v1, material);
    }

    // Draw bottom part (v1.y to v2.y) - Flat top triangle
    if (v1.y < v2.y) {
        drawScanlines(v1.y, v2.y, v1, v2, v0, v2, material); // Note edge AC is still v0 -> v2
    }
}


void Renderer::drawScanlines(int yStart, int yEnd, const ScreenVertex& vStartA, const ScreenVertex& vEndA,
            const ScreenVertex& vStartB, const ScreenVertex& vEndB, const Material& material) {

    float dyA = static_cast<float>(vEndA.y - vStartA.y);
    float dyB = static_cast<float>(vEndB.y - vStartB.y);
    // Avoid division by zero
    float invDyA = (std::abs(dyA) > 1e-6f) ? 1.0f / dyA : 0.0f;
    float invDyB = (std::abs(dyB) > 1e-6f) ? 1.0f / dyB : 0.0f;

    // Clamp yStart and yEnd to framebuffer bounds
    yStart = std::max(0, yStart);
    yEnd = std::min(framebuffer.getHeight() - 1, yEnd);

    for (int y = yStart; y <= yEnd; ++y) {
        // Interpolation factors along edges
        float tA = (y - vStartA.y) * invDyA;
        float tB = (y - vStartB.y) * invDyB;

        // Interpolate attributes along edges
        float xa = vStartA.x + (vEndA.x - vStartA.x) * tA;
        float xb = vStartB.x + (vEndB.x - vStartB.x) * tB;
        float za = vStartA.z + (vEndA.z - vStartA.z) * tA;
        float zb = vStartB.z + (vEndB.z - vStartB.z) * tB;
        float invWa = vStartA.invW + (vEndA.invW - vStartA.invW) * tA;
        float invWb = vStartB.invW + (vEndB.invW - vStartB.invW) * tB;

        // Interpolate Varyings perspective-correctly along edges A and B
        Varyings varyingsA = interpolateVaryings(tA, vStartA.varyings, vEndA.varyings, vStartA.invW, vEndA.invW);
        Varyings varyingsB = interpolateVaryings(tB, vStartB.varyings, vEndB.varyings, vStartB.invW, vEndB.invW);

        // Ensure xa <= xb
        if (xa > xb) {
            std::swap(xa, xb);
            std::swap(za, zb);
            std::swap(invWa, invWb);
            std::swap(varyingsA, varyingsB);
        }

        int xStart = std::max(0, static_cast<int>(std::ceil(xa)));
        int xEnd = std::min(framebuffer.getWidth() - 1, static_cast<int>(std::floor(xb)));

        float dx = xb - xa;
        float invDx = (std::abs(dx) > 1e-6f) ? 1.0f / dx : 0.0f;

        for (int x = xStart; x <= xEnd; ++x) {
            // Calculate interpolation factor 'tHoriz' across the scanline
            float tHoriz = (x - xa) * invDx;
             // Interpolate depth linearly across scanline
            float depth = za + (zb - za) * tHoriz;

            // Check depth buffer *before* expensive fragment shader
            if (depth >= framebuffer.getDepth(x, y)) { // Using >= because we mapped depth to [0,1] and buffer initialized to 0
                continue; // Occluded
            }
            
            // Interpolate 1/W linearly across scanline
            float currentInvW = invWa + (invWb - invWa) * tHoriz;
            // Interpolate varyings perspective-correctly across the scanline
            Varyings finalVaryings = interpolateVaryings(tHoriz, varyingsA, varyingsB, invWa, invWb);
            // --- Fragment Shader ---
            vec3f fragmentColor;
            if (material.shader->fragment(finalVaryings, fragmentColor)) {
                // Write to framebuffer if fragment not discarded
                framebuffer.setPixel(x, y, fragmentColor, depth);
            }
        }
    }
}
