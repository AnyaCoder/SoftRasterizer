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

int Framebuffer::interpolate(int x1, int y1, int x2, int y2, int y) {
    if (y1 == y2) return x1;
    return x1 + (x2 - x1) * (y - y1) / (y2 - y1);
}

void Framebuffer::drawTriangle(int x0, int y0, int x1, int y1, int x2, int y2, const Vector3<float>& color) {
    // Sort vertices by y-coordinate (y0 <= y1 <= y2)
    if (y0 > y1) { std::swap(x0, x1); std::swap(y0, y1); }
    if (y0 > y2) { std::swap(x0, x2); std::swap(y0, y2); }
    if (y1 > y2) { std::swap(x1, x2); std::swap(y1, y2); }

    // Handle degenerate triangles
    if (y0 == y2) return;

    // Draw top part of triangle (y0 to y1)
    for (int y = y0; y <= y1; y++) {
        int xa = interpolate(x0, y0, x2, y2, y);
        int xb = interpolate(x0, y0, x1, y1, y);
        if (xa > xb) std::swap(xa, xb);
        for (int x = xa; x <= xb; x++) {
            setPixel(x, y, color);
        }
    }

    // Draw bottom part of triangle (y1 to y2)
    for (int y = y1; y <= y2; y++) {
        int xa = interpolate(x0, y0, x2, y2, y);
        int xb = interpolate(x1, y1, x2, y2, y);
        if (xa > xb) std::swap(xa, xb);
        for (int x = xa; x <= xb; x++) {
            setPixel(x, y, color);
        }
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
