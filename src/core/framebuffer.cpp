// src/core/framebuffer.cpp
#include "core/framebuffer.h"
#include <iostream>

Framebuffer::Framebuffer(int w, int h, ThreadPool& tp) 
    : width(w), height(h), pixels(w * h), 
        zBuffer(w * h, 1.0f), pixelLocks(LOCK_POOL_SIZE), threadPool(tp) {

}

void Framebuffer::clear(const vec3f& color) {
    std::fill(pixels.begin(), pixels.end(), color);
}

void Framebuffer::clearZBuffer() {
    std::fill(zBuffer.begin(), zBuffer.end(), 1.0f);
}

void Framebuffer::setPixel(int x, int y, const vec3f& color, float depth) {
    // 使用像素锁保护深度测试和像素写入
    std::lock_guard<std::mutex> lock(pixelLocks[getLockIndex(x, y)]);
    int index = y * width + x;
    // 改为小于测试：深度值越小（更近）越能覆盖已有像素
    if (depth < zBuffer[index]) {  // 右手系，z值越小表示越近
        zBuffer[index] = depth;
        pixels[index] = color;
    }
}

const std::vector<vec3f>& Framebuffer::getPixels() const {
    return pixels;
}

float Framebuffer::getDepth(int x, int y) { 
    return zBuffer[y * width + x];
}

void Framebuffer::flipHorizontal() {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width / 2; x++) {
            std::swap(pixels[y * width + x], pixels[y * width + (width - 1 - x)]);
        }
    }
}

void Framebuffer::flipVertical() {
#ifdef MultiThreading
    uint32_t numThreads = threadPool.getNumThreads();
    numThreads = std::max(1u, numThreads);
    int rowsPerThread = (height / 2 + numThreads - 1) / numThreads; // Ceiling division
    
    for (int startY = 0; startY < height / 2; startY += rowsPerThread) {
        int endY = std::min(startY + rowsPerThread, height / 2);
        threadPool.enqueue([this, startY, endY]() {
            for (int y = startY; y < endY; ++y) {
                for (int x = 0; x < width; ++x) {
                    std::swap(pixels[y * width + x], pixels[(height - 1 - y) * width + x]);
                }
            }
        });
    }
    threadPool.waitForCompletion();
#else
    for (int y = 0; y < height / 2; y++) {
        for (int x = 0; x < width; x++) {
            std::swap(pixels[y * width + x], pixels[(height - 1 - y) * width + x]);
        }
    }
#endif
}

