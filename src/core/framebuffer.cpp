// src/core/framebuffer.cpp
#include "core/framebuffer.h"

Framebuffer::Framebuffer(int w, int h) : width(w), height(h), pixels(w * h), zBuffer(w * h, 1.0f) {}

void Framebuffer::clear(const vec3f& color) {
    std::fill(pixels.begin(), pixels.end(), color);
}

void Framebuffer::clearZBuffer() {
    std::fill(zBuffer.begin(), zBuffer.end(), 1.0f);
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

