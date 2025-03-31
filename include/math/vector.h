#pragma once

template<typename T>
struct Vector2 {
    T x, y;
    
    Vector2() : x(0), y(0) {}
    Vector2(T x, T y) : x(x), y(y) {}
    
    Vector2 operator+(const Vector2& v) const { return Vector2(x + v.x, y + v.y); }
    Vector2 operator-(const Vector2& v) const { return Vector2(x - v.x, y - v.y); }
    Vector2 operator*(T s) const { return Vector2(x * s, y * s); }
    T dot(const Vector2& v) const { return x * v.x + y * v.y; }
    float length() const { return sqrtf(x * x + y * y); }
    Vector2 normalized() const { float l = length(); return Vector2(x / l, y / l); }
};

template<typename T>
struct Vector3 {
    T x, y, z;
    
    Vector3() : x(0), y(0), z(0) {}
    Vector3(T x, T y, T z) : x(x), y(y), z(z) {}
    
    Vector3 operator+(const Vector3& v) const { return Vector3(x + v.x, y + v.y, z + v.z); }
    Vector3 operator-(const Vector3& v) const { return Vector3(x - v.x, y - v.y, z - v.z); }
    Vector3 operator*(T s) const { return Vector3(x * s, y * s, z * s); }
    T dot(const Vector3& v) const { return x * v.x + y * v.y + z * v.z; }
    Vector3 cross(const Vector3& v) const {
        return Vector3(
            y * v.z - z * v.y,
            z * v.x - x * v.z,
            x * v.y - y * v.x
        );
    }
    float length() const { return sqrtf(x * x + y * y + z * z); }
    Vector3 normalized() const { float l = length(); return Vector3(x / l, y / l, z / l); }
};

template<typename T>
struct Vector4 {
    T x, y, z, w;
    
    Vector4() : x(0), y(0), z(0), w(0) {}
    Vector4(T x, T y, T z, T w) : x(x), y(y), z(z), w(w) {}
    Vector4(const Vector3<T>& v, T w) : x(v.x), y(v.y), z(v.z), w(w) {}
    
    Vector3<T> xyz() const { return Vector3<T>(x, y, z); }
};
