#include "core/framebuffer.h"

Framebuffer::Framebuffer(int w, int h) : width(w), height(h), pixels(w * h) {}

void Framebuffer::clear(const Vector3<float>& color) {
    std::fill(pixels.begin(), pixels.end(), color);
}

void Framebuffer::setPixel(int x, int y, const Vector3<float>& color) {
    if (x >= 0 && x < width && y >= 0 && y < height) {
        pixels[y * width + x] = color;
    }
}
