// // src/core/renderer.cpp
#include "core/renderer.h"
#include "core/camera.h" // Include camera definition

Renderer::Renderer(Framebuffer& fb) : framebuffer(fb) {}

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


void Renderer::drawModel(Model& model, const mat4& modelMatrix, const Material& material) {
    if (!material.shader) {
        std::cerr << "Error: No shader set for rendering!" << std::endl;
        return;
    }
    
    if (!material.normalTexture.empty() && model.numTangents() == 0) {
        std::cerr << "Warning: Material has normal map but model tangents not calculated/loaded. Call model.calculateTangents()." << std::endl;
    }

    // --- Setup Uniforms ---
    material.shader->uniform_ModelMatrix = modelMatrix;
    material.shader->uniform_ViewMatrix = viewMatrix;
    material.shader->uniform_ProjectionMatrix = projectionMatrix;
    material.shader->uniform_MVP = projectionMatrix * viewMatrix * modelMatrix;
    
    material.shader->uniform_NormalMatrix = modelMatrix.inverse().transpose();
    material.shader->uniform_CameraPosition = cameraPosition;
    material.shader->uniform_Lights = lights;
    
    material.shader->uniform_AmbientColor = material.ambientColor;
    material.shader->uniform_DiffuseColor = material.diffuseColor;
    material.shader->uniform_SpecularColor = material.specularColor;
    material.shader->uniform_Shininess = material.shininess;
    
    material.shader->uniform_DiffuseTexture = material.diffuseTexture;
    material.shader->uniform_NormalTexture = material.normalTexture; // Set normal map

    // --- Vertex Processing & Triangle Assembly ---
    for (int i = 0; i < model.numFaces(); ++i) {
        Model::Face face = model.getFace(i);
        ScreenVertex screenVertices[3];
        Varyings varyings[3];
        bool triangleVisible = false; // 改为 false，默认不可见

        for (int j = 0; j < 3; ++j) {
            VertexInput vInput;

            int vertIdx = face.vertIndex[j];
            int uvIdx = face.uvIndex[j];
            int normIdx = face.normIndex[j];

            vInput.position  = model.getVertex(vertIdx);
            vInput.normal    = model.getNormal(normIdx);
            vInput.uv        = model.getUV(uvIdx);
            vInput.tangent   = model.getTangent(vertIdx);
            vInput.bitangent = model.getBitangent(vertIdx);

            varyings[j]     = material.shader->vertex(vInput);

            // 检查是否在裁剪空间范围内
            float w = varyings[j].clipPosition.w;
            float z = varyings[j].clipPosition.z;
            if (w > 0 && z >= -w && z <= w) { // 仅当顶点在近裁剪面内时标记可见
                triangleVisible = true;
            }
        }

        // 如果所有顶点都在近裁剪面外，则跳过
        if (!triangleVisible) {
            continue;
        }

        for (int j = 0; j < 3; ++j) {
            if (varyings[j].clipPosition.w <= 0) continue; // 防止除以0或负数
            float invW = 1.0f / varyings[j].clipPosition.w;
            vec3f ndcPos = {
                varyings[j].clipPosition.x * invW,
                varyings[j].clipPosition.y * invW,
                varyings[j].clipPosition.z * invW
            };

            screenVertices[j].x = static_cast<int>((ndcPos.x + 1.0f) * 0.5f * framebuffer.getWidth());
            screenVertices[j].y = static_cast<int>((ndcPos.y + 1.0f) * 0.5f * framebuffer.getHeight());
            screenVertices[j].z = (ndcPos.z + 1.0f) * 0.5f; // 直接使用 NDC z 映射到 [0, 1]
            screenVertices[j].invW = invW;
            screenVertices[j].varyings = varyings[j];

        }

        // 背面剔除和光栅化逻辑保持不变
        if (triangleVisible) {
            vec2f p0 = { (float)screenVertices[0].x, (float)screenVertices[0].y };
            vec2f p1 = { (float)screenVertices[1].x, (float)screenVertices[1].y };
            vec2f p2 = { (float)screenVertices[2].x, (float)screenVertices[2].y };
            float signedArea = (p1.x - p0.x) * (p2.y - p0.y) - (p2.x - p0.x) * (p1.y - p0.y);
            if (signedArea < 0) {
                continue;
            }
            drawTriangle(screenVertices[0], screenVertices[1], screenVertices[2], material);
        }
    }
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

    // Clamp yStart and yEnd to framebuffer bounds
    yStart = std::max(0, yStart);
    yEnd = std::min(framebuffer.getHeight() - 1, yEnd);

    for (int y = yStart; y <= yEnd; ++y) {
         // Calculate interpolation factors 't' along edges A (vStartA -> vEndA) and B (vStartB -> vEndB)
        float tA = (std::abs(dyA) > 1e-6f) ? static_cast<float>(y - vStartA.y) / dyA : 0.0f;
        float tB = (std::abs(dyB) > 1e-6f) ? static_cast<float>(y - vStartB.y) / dyB : 0.0f;

        tA = std::max(0.0f, std::min(1.0f, tA)); // Clamp t
        tB = std::max(0.0f, std::min(1.0f, tB));

        // Interpolate X, Z, 1/W linearly along edges A and B
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

        for (int x = xStart; x <= xEnd; ++x) {
            // Calculate interpolation factor 'tHoriz' across the scanline
            float tHoriz = (std::abs(dx) > 1e-6f) ? (static_cast<float>(x) - xa) / dx : 0.0f;
            tHoriz = std::max(0.0f, std::min(1.0f, tHoriz)); // Clamp

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
