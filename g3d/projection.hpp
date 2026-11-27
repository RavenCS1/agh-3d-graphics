#pragma once
// g3d/projection.hpp - projekcja perspektywiczna / ortograficzna (lab 2, zad. 8).
//
//   g3d::Projection proj;                    // perspektywa, fov 45
//   proj.apply(w, h);                        // po utworzeniu okna i przy Resized
//   if (proj.handleKey(key)) proj.apply(w, h);   // 9/0 typ, V/B fov

#include "gl.hpp"
#include "math.hpp"

namespace g3d {

struct Projection {
    bool perspective = true;
    float fov = 45.0f;               // pionowy kat widzenia w stopniach
    float minFov = 25.0f, maxFov = 90.0f, fovStep = 5.0f;
    float orthoHalfHeight = 1.75f;   // polowa wysokosci bryly widzenia w rzucie rownoleglym
    double zNear = 0.1, zFar = 100.0;

    // Ustawia viewport i macierz GL_PROJECTION, potem wraca do GL_MODELVIEW.
    void apply(unsigned width, unsigned height) const {
        const unsigned w = width > 0 ? width : 1u;    // ochrona przed dzieleniem przez zero
        const unsigned h = height > 0 ? height : 1u;
        const double aspect = static_cast<double>(w) / static_cast<double>(h);

        glViewport(0, 0, static_cast<GLsizei>(w), static_cast<GLsizei>(h));
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        if (perspective) {
            gluPerspective(fov, aspect, zNear, zFar);
        } else {
            glOrtho(-orthoHalfHeight * aspect, orthoHalfHeight * aspect,
                    -orthoHalfHeight, orthoHalfHeight, zNear, zFar);
        }
        glMatrixMode(GL_MODELVIEW);
    }

    // 9 - perspektywa, 0 - rzut ortograficzny, V/B - fov -/+ (tylko w perspektywie).
    // Zwraca true, gdy trzeba ponownie wywolac apply().
    bool handleKey(sf::Keyboard::Key key) {
        switch (key) {
        case sf::Keyboard::Num9: perspective = true; return true;
        case sf::Keyboard::Num0: perspective = false; return true;
        case sf::Keyboard::V:
            if (!perspective) return false;
            fov = clampf(fov - fovStep, minFov, maxFov);
            return true;
        case sf::Keyboard::B:
            if (!perspective) return false;
            fov = clampf(fov + fovStep, minFov, maxFov);
            return true;
        default:
            return false;
        }
    }
};

} // namespace g3d
