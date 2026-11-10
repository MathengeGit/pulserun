#pragma once
#include "math/vec2.h"

// 3x3 matrix for 2D homogeneous transforms. A point (x, y) is promoted to the
// column vector (x, y, 1), so translation lives in the third column.
//
// Storage is column-major: m[col][row]. Composition reads right to left:
//   (T * R * S) * v  ==  T * (R * (S * v))   -> scale first, then rotate, then translate.
//
// Screen space has y pointing down, so a positive angle turns clockwise on screen
// (the same convention as rotate() in vec2.h).
struct Mat3 {
    float m[3][3];

    static Mat3 identity();
    static Mat3 translate(float dx, float dy);
    static Mat3 rotate(float radians);
    static Mat3 scale(float sx, float sy);
    // Rotate/scale about `pivot` instead of the origin: T(pivot) * R * T(-pivot).
    static Mat3 rotateAbout(Vec2 pivot, float radians);
    static Mat3 scaleAbout(Vec2 pivot, float sx, float sy);

    float at(int row, int col) const { return m[col][row]; }

    Mat3 operator*(const Mat3& o) const;
    Vec2 operator*(const Vec2& v) const;  // transforms a point (w = 1)
    Vec2 applyVector(const Vec2& v) const;  // transforms a direction (w = 0, ignores translation)

    float determinant() const;
    // Inverse via the adjugate. Returns identity() for a singular matrix (|det| < 1e-12).
    Mat3 inverse() const;
};
