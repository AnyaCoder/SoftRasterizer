#pragma once

struct Vertex {
    int x = 0;
    int y = 0;
    float z = 0.0f;
    float u = 0.0f;
    float v = 0.0f;
    
    Vertex() = default;
    Vertex(int x, int y, float z, float u = 0.0f, float v = 0.0f) 
        : x(x), y(y), z(z), u(u), v(v) {}
};