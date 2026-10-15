#pragma once
#include <cmath>

struct Vec2 {
    float x = 0, y = 0;
    Vec2() = default;
    Vec2(float x_, float y_) : x(x_), y(y_) {}
};

inline Vec2 operator+(Vec2 a, Vec2 b) { return Vec2(a.x + b.x, a.y + b.y); }
inline Vec2 operator-(Vec2 a, Vec2 b) { return Vec2(a.x - b.x, a.y - b.y); }
inline Vec2 operator*(Vec2 a, float k) { return Vec2(a.x * k, a.y * k); }
inline Vec2 operator*(float k, Vec2 a) { return Vec2(a.x * k, a.y * k); }

inline float dot(Vec2 a, Vec2 b) { return a.x * b.x + a.y * b.y; }
inline float length(Vec2 a) { return std::sqrt(dot(a, a)); }
inline Vec2 lerp(Vec2 a, Vec2 b, float t) { return a + (b - a) * t; }

// Rotate by `angle` radians. With y pointing down this turns clockwise on screen.
inline Vec2 rotate(Vec2 v, float angle) {
    const float c = std::cos(angle), s = std::sin(angle);
    return Vec2(v.x * c - v.y * s, v.x * s + v.y * c);
}

constexpr float kPi = 3.14159265358979f;
