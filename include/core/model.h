#pragma once
#include <vector>
#include <string>
#include "math/vector.h"

class Framebuffer;

class Model {
public:
    std::vector<Vector3<float>> vertices;
    std::vector<std::vector<int>> faces;

    bool loadFromObj(const std::string& filename);
    void renderWireframe(Framebuffer& fb, const Vector3<float>& color);
};
