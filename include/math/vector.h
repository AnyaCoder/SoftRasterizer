// include/math/vector.h
#pragma once
#include <vector>
#include <iostream>

template<typename T>
struct Vector2 {
    T x, y;
    
    Vector2() : x(0), y(0) {}
    Vector2(T x, T y) : x(x), y(y) {}
    
    Vector2 operator+(const Vector2& v) const { return Vector2(x + v.x, y + v.y); }
    Vector2 operator-(const Vector2& v) const { return Vector2(x - v.x, y - v.y); }
    Vector2 operator*(T s) const { return Vector2(x * s, y * s); }
    Vector2& operator*=(T s) { x *= s; y *= s; return *this; }
    T dot(const Vector2& v) const { return x * v.x + y * v.y; }
    float lengthSq() const { return x * x + y * y; }
    Vector2 normalized() const { float l = sqrtf(lengthSq()); return Vector2(x / l, y / l); }
};

template<typename T>
struct Vector3 {
    T x, y, z;
    
    Vector3() : x(0), y(0), z(0) {}
    Vector3(T x, T y, T z) : x(x), y(y), z(z) {}
    Vector3(const std::vector<T>& v) { x = v[0]; y = v[1]; z = v[2];}
    
    Vector3 operator+(const Vector3& v) const { return Vector3(x + v.x, y + v.y, z + v.z); }
    Vector3& operator+=(const Vector3& v) { x += v.x; y += v.y; z += v.z; return *this; }
    Vector3& operator=(const std::vector<T>& v) { x = v[0]; y = v[1]; z = v[2]; return *this;}
    Vector3 operator-() const { return Vector3(-x, -y, -z); }
    Vector3 operator-(const Vector3& v) const { return Vector3(x - v.x, y - v.y, z - v.z); }
    Vector3 operator*(T s) const { return Vector3(x * s, y * s, z * s); }
    Vector3 operator/(T s) const { return Vector3(x / s, y / s, z / s); }
    Vector3 operator*(const Vector3& v) { return Vector3(x * v.x, y * v.y, z * v.z); }
    T dot(const Vector3& v) const { return x * v.x + y * v.y + z * v.z; }
    Vector3 cross(const Vector3& v) const {
        return Vector3(
            y * v.z - z * v.y,
            z * v.x - x * v.z,
            x * v.y - y * v.x
        );
    }
    float lengthSq() const { return x * x + y * y + z * z; }
    void normalize() { float invL = 1.0f / sqrtf(lengthSq());  x *= invL; y *= invL; z *= invL; }
    Vector3 normalized() const { float invL = 1.0f / sqrtf(lengthSq()); return Vector3(x * invL, y * invL, z * invL); }
    friend std::ostream& operator<< (std::ostream& os, const Vector3& v) {
        os << "vec3f: (" << v.x << ", " << v.y << ", " << v.z << ")" << std::endl;
        return os;
    }
};

template<typename T>
struct Vector4 {
    T x, y, z, w;
    
    Vector4() : x(0), y(0), z(0), w(0) {}
    Vector4(T x, T y, T z, T w) : x(x), y(y), z(z), w(w) {}
    Vector4 operator+(const Vector4& v) const { return Vector4(x + v.x, y + v.y, z + v.z, w + v.w); }
    Vector4 operator-(const Vector4& v) const { return Vector4(x - v.x, y - v.y, z - v.z, w - v.w); }
    Vector4 operator*(const Vector4& v) const { return Vector4(x * v.x, y * v.y, z * v.z, w * v.w); }
    Vector4 operator*(T s) const { return Vector4(x * s, y * s, z * s, w * s); }
    Vector4(const Vector3<T>& v, T w) : x(v.x), y(v.y), z(v.z), w(w) {}
    
    Vector3<T> xyz() const { return Vector3<T>(x, y, z); }
};

using vec2f = Vector2<float>;
using vec3f = Vector3<float>;
using vec4f = Vector4<float>;
using vec2i = Vector2<int>;