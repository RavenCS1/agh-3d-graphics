#pragma once
// g3d/collision.hpp - kolizje prostopadloscianow rownoleglych do osi (AABB) i ruch "z poslizgiem".
//
//   auto depth = [&](const g3d::Vec3& p) {                 // jak gleboko obiekt w p siedzi w przeszkodach
//       return g3d::penetration(g3d::Aabb::around(p, halfSize), obstacle);
//   };
//   pos = g3d::slideMove(pos, velocity * dt, depth);        // os po osi; zablokowana os stoi,
//                                                            // pozostale nadal dzialaja

#include <algorithm>

#include "math.hpp"

namespace g3d {

struct Aabb {
    Vec3 min, max;
    Aabb() {}
    Aabb(const Vec3& mn, const Vec3& mx) : min(mn), max(mx) {}
    static Aabb around(const Vec3& center, const Vec3& half) { return Aabb(center - half, center + half); }
};

// Najmniejsze wnikniecie dwoch pudelek (0 = nie stykaja sie). Najmniejsza os to
// najkrotsza droga wyjscia z kolizji.
inline float penetration(const Aabb& a, const Aabb& b) {
    const float ox = std::min(a.max.x, b.max.x) - std::max(a.min.x, b.min.x);
    const float oy = std::min(a.max.y, b.max.y) - std::max(a.min.y, b.min.y);
    const float oz = std::min(a.max.z, b.max.z) - std::max(a.min.z, b.min.z);
    if (ox <= 0.0f || oy <= 0.0f || oz <= 0.0f) return 0.0f;
    return std::min(ox, std::min(oy, oz));
}

// Przesuwa punkt o delta osobno w X, Y i Z. Ruch w danej osi jest odrzucany, jesli poglebia
// kolizje (depthAt rosnie). Dzieki temu obiekt slizga sie po przeszkodzie, a jesli przeszkoda
// pojawi sie w miejscu obiektu, mozna z niej wyjechac. blocked[i] = true dla zablokowanej osi.
template <class DepthFn>
Vec3 slideMove(Vec3 pos, const Vec3& delta, DepthFn depthAt, bool blocked[3] = nullptr) {
    const float d[3] = {delta.x, delta.y, delta.z};
    for (int axis = 0; axis < 3; ++axis) {
        if (blocked) blocked[axis] = false;
        if (d[axis] == 0.0f) continue;
        Vec3 next = pos;
        if (axis == 0) next.x += d[0];
        if (axis == 1) next.y += d[1];
        if (axis == 2) next.z += d[2];
        if (depthAt(next) <= depthAt(pos) + 1.0e-4f) pos = next;
        else if (blocked) blocked[axis] = true;
    }
    return pos;
}

} // namespace g3d
