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

void Framebuffer::drawLine(int x0, int y0, int x1, int y1, const Vector3<float>& color) {
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
