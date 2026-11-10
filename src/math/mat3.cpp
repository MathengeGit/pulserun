#include "math/mat3.h"
#include <cmath>

namespace {
Mat3 fromRows(float a, float b, float c, float d, float e, float f, float g, float h, float i) {
    Mat3 r;
    r.m[0][0] = a; r.m[1][0] = b; r.m[2][0] = c;
    r.m[0][1] = d; r.m[1][1] = e; r.m[2][1] = f;
    r.m[0][2] = g; r.m[1][2] = h; r.m[2][2] = i;
    return r;
}
}  // namespace

Mat3 Mat3::identity() { return fromRows(1, 0, 0, 0, 1, 0, 0, 0, 1); }

Mat3 Mat3::translate(float dx, float dy) { return fromRows(1, 0, dx, 0, 1, dy, 0, 0, 1); }

Mat3 Mat3::rotate(float radians) {
    const float c = std::cos(radians), s = std::sin(radians);
    return fromRows(c, -s, 0, s, c, 0, 0, 0, 1);
}

Mat3 Mat3::scale(float sx, float sy) { return fromRows(sx, 0, 0, 0, sy, 0, 0, 0, 1); }

Mat3 Mat3::rotateAbout(Vec2 p, float radians) { return translate(p.x, p.y) * rotate(radians) * translate(-p.x, -p.y); }

Mat3 Mat3::scaleAbout(Vec2 p, float sx, float sy) { return translate(p.x, p.y) * scale(sx, sy) * translate(-p.x, -p.y); }

Mat3 Mat3::operator*(const Mat3& o) const {
    Mat3 r;
    for (int col = 0; col < 3; ++col)
        for (int row = 0; row < 3; ++row) {
            float sum = 0;
            for (int k = 0; k < 3; ++k) sum += at(row, k) * o.at(k, col);
            r.m[col][row] = sum;
        }
    return r;
}

Vec2 Mat3::operator*(const Vec2& v) const {
    const float x = at(0, 0) * v.x + at(0, 1) * v.y + at(0, 2);
    const float y = at(1, 0) * v.x + at(1, 1) * v.y + at(1, 2);
    const float w = at(2, 0) * v.x + at(2, 1) * v.y + at(2, 2);
    // Affine matrices keep w == 1; divide anyway so a general matrix still works.
    if (w != 0.0f && w != 1.0f) return Vec2(x / w, y / w);
    return Vec2(x, y);
}

Vec2 Mat3::applyVector(const Vec2& v) const {
    return Vec2(at(0, 0) * v.x + at(0, 1) * v.y, at(1, 0) * v.x + at(1, 1) * v.y);
}

float Mat3::determinant() const {
    return at(0, 0) * (at(1, 1) * at(2, 2) - at(1, 2) * at(2, 1)) -
           at(0, 1) * (at(1, 0) * at(2, 2) - at(1, 2) * at(2, 0)) +
           at(0, 2) * (at(1, 0) * at(2, 1) - at(1, 1) * at(2, 0));
}

Mat3 Mat3::inverse() const {
    const float det = determinant();
    if (std::fabs(det) < 1e-12f) return identity();
    const float k = 1.0f / det;
    // inverse = adjugate / det, adjugate = transpose of the cofactor matrix.
    auto a = [&](int r, int c) { return at(r, c); };
    return fromRows(
        (a(1, 1) * a(2, 2) - a(1, 2) * a(2, 1)) * k, (a(0, 2) * a(2, 1) - a(0, 1) * a(2, 2)) * k,
        (a(0, 1) * a(1, 2) - a(0, 2) * a(1, 1)) * k, (a(1, 2) * a(2, 0) - a(1, 0) * a(2, 2)) * k,
        (a(0, 0) * a(2, 2) - a(0, 2) * a(2, 0)) * k, (a(0, 2) * a(1, 0) - a(0, 0) * a(1, 2)) * k,
        (a(1, 0) * a(2, 1) - a(1, 1) * a(2, 0)) * k, (a(0, 1) * a(2, 0) - a(0, 0) * a(2, 1)) * k,
        (a(0, 0) * a(1, 1) - a(0, 1) * a(1, 0)) * k);
}
