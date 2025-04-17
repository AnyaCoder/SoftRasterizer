// include/core/framebuffer.h
#pragma once
#include <vector>
#include <string>
#include <mutex>
#include "math/vector.h"
#include "core/texture.h"
#include "core/vertex.h"
#include "core/threadpool.h"


class ThreadPool;

class Framebuffer {
public:
    Framebuffer(int w, int h, ThreadPool& tp);

    void clearZBuffer();
    void clear(const vec3f& color);

    // Modified setPixel takes depth in [0, 1] range (0=near, 1=far)
    void setPixel(int x, int y, const vec3f& color, float depth = 0.0f);
    const std::vector<vec3f>& getPixels() const;

    // Getter for depth buffer value
    float getDepth(int x, int y);

    int getWidth() const { return width; }
    int getHeight() const { return height; }

    bool saveToTGA(const std::string& filename);
    
    void flipHorizontal();
    void flipVertical();

private:
    int width;
    int height;
    std::vector<vec3f> pixels;
    std::vector<float> zBuffer;  // Depth buffer
    ThreadPool& threadPool;

    static constexpr int LOCK_POOL_SIZE = 2047;
    std::vector<std::mutex> pixelLocks;

    int getLockIndex(int x, int y) const {
        return (x * 13331 + y) % LOCK_POOL_SIZE;
    }
};
