#pragma once
// g3d/gl_state.hpp - obiekty RAII, ktore zmieniaja stan OpenGL tylko na czas jednego bloku
// i same przywracaja poprzednie ustawienia (nawet przy wczesnym return).
//
//   {
//       g3d::ScopedMatrix m;                       // glPushMatrix / glPopMatrix
//       g3d::ScopedCapability noLight(GL_LIGHTING, false);
//       g3d::ScopedLineWidth w(2.0f);
//       ... rysowanie ...
//   }                                              // tu wszystko wraca do stanu sprzed bloku

#include "gl.hpp"

namespace g3d {

class ScopedMatrix {
public:
    ScopedMatrix() { glPushMatrix(); }
    ~ScopedMatrix() { glPopMatrix(); }
    ScopedMatrix(const ScopedMatrix&) = delete;
    ScopedMatrix& operator=(const ScopedMatrix&) = delete;
};

// Wlacza albo wylacza dana funkcje OpenGL (GL_LIGHTING, GL_DEPTH_TEST, GL_TEXTURE_2D...).
class ScopedCapability {
public:
    ScopedCapability(GLenum cap, bool enable) : cap_(cap), wasEnabled_(glIsEnabled(cap) == GL_TRUE) {
        if (enable) glEnable(cap); else glDisable(cap);
    }
    ~ScopedCapability() { if (wasEnabled_) glEnable(cap_); else glDisable(cap_); }
    ScopedCapability(const ScopedCapability&) = delete;
    ScopedCapability& operator=(const ScopedCapability&) = delete;
private:
    GLenum cap_;
    bool wasEnabled_;
};

class ScopedLineWidth {
public:
    explicit ScopedLineWidth(float width) { glGetFloatv(GL_LINE_WIDTH, &previous_); glLineWidth(width); }
    ~ScopedLineWidth() { glLineWidth(previous_); }
    ScopedLineWidth(const ScopedLineWidth&) = delete;
    ScopedLineWidth& operator=(const ScopedLineWidth&) = delete;
private:
    GLfloat previous_;
};

class ScopedPointSize {
public:
    explicit ScopedPointSize(float size) { glGetFloatv(GL_POINT_SIZE, &previous_); glPointSize(size); }
    ~ScopedPointSize() { glPointSize(previous_); }
    ScopedPointSize(const ScopedPointSize&) = delete;
    ScopedPointSize& operator=(const ScopedPointSize&) = delete;
private:
    GLfloat previous_;
};

// Polprzezroczystosc: GL_BLEND z typowa funkcja mieszania i wylaczony ZAPIS glebokosci.
// Test glebokosci zostaje aktywny, wiec nieprzezroczyste obiekty nadal zaslaniaja warstwe.
// Przezroczyste obiekty rysuj po nieprzezroczystych.
class ScopedTransparency {
public:
    ScopedTransparency() : blend_(GL_BLEND, true), lighting_(GL_LIGHTING, false) {
        glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMask_);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
    }
    ~ScopedTransparency() { glDepthMask(depthMask_); }
    ScopedTransparency(const ScopedTransparency&) = delete;
    ScopedTransparency& operator=(const ScopedTransparency&) = delete;
private:
    ScopedCapability blend_;
    ScopedCapability lighting_;
    GLboolean depthMask_;
};

} // namespace g3d
