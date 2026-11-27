#pragma once
// g3d/math.hpp - stale, pomocnicze funkcje skalarne i prosty wektor 3D.

#include <algorithm>
#include <cmath>

namespace g3d {

const float PI = 3.14159265358979323846f;

inline float clampf(float v, float lo, float hi) { return std::max(lo, std::min(v, hi)); }
inline float degToRad(float deg) { return deg * PI / 180.0f; }
inline float radToDeg(float rad) { return rad * 180.0f / PI; }
inline float lerp(float a, float b, float t) { return a + (b - a) * t; }

// Wygladzanie wykladnicze niezalezne od liczby klatek: wartosc zbliza sie do celu
// z "szybkoscia" rate [1/s]. Przy rate = 6 po 0.5 s zostaje ok. 5% roznicy.
inline float smoothTowards(float value, float target, float rate, float dt) {
    return target + (value - target) * std::exp(-rate * dt);
}

// Najkrotsza roznica katow w radianach, wynik w przedziale [-PI, PI].
inline float angleDelta(float from, float to) {
    float d = std::fmod(to - from + PI, 2.0f * PI);
    if (d < 0.0f) d += 2.0f * PI;
    return d - PI;
}

struct Vec3 {
    float x, y, z;
    Vec3() : x(0.0f), y(0.0f), z(0.0f) {}
    Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
};

inline Vec3 operator+(const Vec3& a, const Vec3& b) { return Vec3(a.x + b.x, a.y + b.y, a.z + b.z); }
inline Vec3 operator-(const Vec3& a, const Vec3& b) { return Vec3(a.x - b.x, a.y - b.y, a.z - b.z); }
inline Vec3 operator-(const Vec3& a) { return Vec3(-a.x, -a.y, -a.z); }
inline Vec3 operator*(const Vec3& a, float s) { return Vec3(a.x * s, a.y * s, a.z * s); }
inline Vec3 operator*(float s, const Vec3& a) { return a * s; }
inline Vec3& operator+=(Vec3& a, const Vec3& b) { a.x += b.x; a.y += b.y; a.z += b.z; return a; }
inline Vec3& operator-=(Vec3& a, const Vec3& b) { a.x -= b.x; a.y -= b.y; a.z -= b.z; return a; }

inline float dot(const Vec3& a, const Vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

// Iloczyn wektorowy: kolejnosc argumentow ma znaczenie (zamiana odwraca wynik).
inline Vec3 cross(const Vec3& a, const Vec3& b) {
    return Vec3(a.y * b.z - a.z * b.y,
                a.z * b.x - a.x * b.z,
                a.x * b.y - a.y * b.x);
}

inline float length(const Vec3& v) { return std::sqrt(dot(v, v)); }

// Normalizacja zabezpieczona przed wektorem zerowym.
inline Vec3 normalize(const Vec3& v, const Vec3& fallback = Vec3(0.0f, 1.0f, 0.0f)) {
    const float len = length(v);
    if (len < 1.0e-6f) return fallback;
    return v * (1.0f / len);
}

} // namespace g3d
