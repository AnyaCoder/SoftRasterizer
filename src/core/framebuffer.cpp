#include "core/framebuffer.h"

Framebuffer::Framebuffer(int w, int h) : width(w), height(h), pixels(w * h), zBuffer(w * h, std::numeric_limits<float>::max()) {}

void Framebuffer::clear(const vec3f& color) {
    std::fill(pixels.begin(), pixels.end(), color);
}

void Framebuffer::clearZBuffer() {
    std::fill(zBuffer.begin(), zBuffer.end(), std::numeric_limits<float>::max());
}

void Framebuffer::setPixel(int x, int y, const vec3f& color, float depth) {
    if (x >= 0 && x < width && y >= 0 && y < height) {
        int index = y * width + x;
        // 改为小于测试：深度值越小（更近）越能覆盖已有像素
        if (depth < zBuffer[index]) {  // 右手系，z值越小表示越近
            zBuffer[index] = depth;
            pixels[index] = color;
        }
    }
}

void Framebuffer::flipHorizontal() {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width / 2; x++) {
            std::swap(pixels[y * width + x], pixels[y * width + (width - 1 - x)]);
        }
    }
}

void Framebuffer::flipVertical() {
    for (int y = 0; y < height / 2; y++) {
        for (int x = 0; x < width; x++) {
            std::swap(pixels[y * width + x], pixels[(height - 1 - y) * width + x]);
        }
    }
}

void Framebuffer::drawScanlines(int yStart, int yEnd, 
                              const Vertex& vStartA, const Vertex& vEndA,
                              const Vertex& vStartB, const Vertex& vEndB,
                              const vec3f& color, const Texture& texture) {
    bool useTexture = !texture.empty();
    
    for (int y = yStart; y <= yEnd; y++) {
        if (y < 0 || y >= height) continue;
        
        int xa = interpolate<int, int>(vStartA.x, vStartA.y, vEndA.x, vEndA.y, y);
        int xb = interpolate<int, int>(vStartB.x, vStartB.y, vEndB.x, vEndB.y, y);
        float za = interpolate<float, int>(vStartA.z, vStartA.y, vEndA.z, vEndA.y, y);
        float zb = interpolate<float, int>(vStartB.z, vStartB.y, vEndB.z, vEndB.y, y);
        
        float wa = interpolate<float, int>(vStartA.w, vStartA.y, vEndA.w, vEndA.y, y);
        float wb = interpolate<float, int>(vStartB.w, vStartB.y, vEndB.w, vEndB.y, y);
        
        float ua = 0, ub = 0, va = 0, vb = 0;
        if (useTexture) {
            ua = interpolate<float, int>(vStartA.u, vStartA.y, vEndA.u, vEndA.y, y);
            ub = interpolate<float, int>(vStartB.u, vStartB.y, vEndB.u, vEndB.y, y);
            va = interpolate<float, int>(vStartA.v, vStartA.y, vEndA.v, vEndA.y, y);
            vb = interpolate<float, int>(vStartB.v, vStartB.y, vEndB.v, vEndB.y, y);
        }

        if (xa > xb) {
            std::swap(xa, xb);
            std::swap(za, zb);
            std::swap(wa, wb);
            if (useTexture) {
                std::swap(ua, ub);
                std::swap(va, vb);
            }
        }
        
        xa = std::max(0, std::min(width-1, xa));
        xb = std::max(0, std::min(width-1, xb));
        
        for (int x = xa; x <= xb; x++) {
            float t = (xb != xa) ? (float)(x - xa)/(xb - xa) : 0.0f;
            float depth = za + (zb - za) * t;
            float w = wa + (wb - wa) * t;
            
            vec3f finalColor = color;
            if (useTexture && w != 0) {
                // 透视校正插值
                float invW = 1.0f / w;
                float u = (ua + (ub - ua) * t) * invW;
                float v = (va + (vb - va) * t) * invW;
                finalColor = texture.sample(u, v) * color;
            }
            
            setPixel(x, y, finalColor, depth);
        }
    }
}


void Framebuffer::drawTriangle(Vertex v0, Vertex v1, Vertex v2,
                     const vec3f& color,
                     const Texture& texture) {
    // Sort vertices by y-coordinate (v0.y <= v1.y <= v2.y)
    if (v0.y > v1.y) { std::swap(v0, v1); }
    if (v0.y > v2.y) { std::swap(v0, v2); }
    if (v1.y > v2.y) { std::swap(v1, v2); }

    // Handle degenerate triangles
    if (v0.y == v2.y) return;

    // Draw top part (v0.y to v1.y)
    if (v0.y < v1.y) {
        drawScanlines(v0.y, v1.y, v0, v2, v0, v1, color, texture);
    }

    // Draw bottom part (v1.y to v2.y)
    if (v1.y < v2.y) {
        drawScanlines(v1.y, v2.y, v0, v2, v1, v2, color, texture);
    }
}

void Framebuffer::drawLine(int x0, int y0, int x1, int y1, const vec3f& color) {
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
            setPixel(y, x, color);
        } else {
            setPixel(x, y, color);
        }
        err -= dy;
        if (err < 0) {
            y += ystep;
            err += dx;
        }
    }
}
