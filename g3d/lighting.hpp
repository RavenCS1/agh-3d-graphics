#pragma once
// g3d/lighting.hpp - klasyczne oswietlenie OpenGL (lab 3, zad. 10-12).
//
// Kolejnosc w klatce:  kamera (gluLookAt)  ->  setLightPosition(...)  ->  transformacje obiektow.
// Pozycja swiatla jest mnozona przez aktualna macierz MODELVIEW w chwili wywolania glLightfv,
// wiec ustawiona po kamerze oznacza punkt/kierunek w ukladzie swiata.

#include "gl.hpp"
#include "math.hpp"

namespace g3d {

struct Rgba {
    GLfloat v[4];
    Rgba(float r, float g, float b, float a = 1.0f) { v[0] = r; v[1] = g; v[2] = b; v[3] = a; }
    const GLfloat* data() const { return v; }
};

// Jednorazowa konfiguracja zrodla swiatla i modelu oswietlenia.
inline void setupLight(GLenum light, const Rgba& ambient, const Rgba& diffuse, const Rgba& specular,
                       const Rgba& globalAmbient = Rgba(0.12f, 0.12f, 0.12f)) {
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmbient.data());
    glLightfv(light, GL_AMBIENT, ambient.data());
    glLightfv(light, GL_DIFFUSE, diffuse.data());
    glLightfv(light, GL_SPECULAR, specular.data());
    glEnable(light);
    glEnable(GL_NORMALIZE);               // normalne poprawne takze po glScalef
}

// w = 1: swiatlo punktowe w punkcie p;  w = 0: swiatlo kierunkowe, p to kierunek DO zrodla.
inline void setLightPosition(GLenum light, const Vec3& p, bool pointLight = true) {
    const GLfloat pos[4] = {p.x, p.y, p.z, pointLight ? 1.0f : 0.0f};
    glLightfv(light, GL_POSITION, pos);
}

// Tlumienie swiatla punktowego: 1 / (c + l*d + q*d^2).
inline void setAttenuation(GLenum light, float constant, float linear, float quadratic) {
    glLightf(light, GL_CONSTANT_ATTENUATION, constant);
    glLightf(light, GL_LINEAR_ATTENUATION, linear);
    glLightf(light, GL_QUADRATIC_ATTENUATION, quadratic);
}

// Material ustawiany jawnie (przy wylaczonym GL_COLOR_MATERIAL).
inline void setMaterial(const Rgba& ambientDiffuse, const Rgba& specular, float shininess) {
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, ambientDiffuse.data());
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular.data());
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, clampf(shininess, 0.0f, 128.0f));
}

// glColor3f steruje skladowymi ambient + diffuse materialu.
inline void useColorMaterial(bool enable) {
    if (enable) {
        glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
        glEnable(GL_COLOR_MATERIAL);
    } else {
        glDisable(GL_COLOR_MATERIAL);
    }
}

} // namespace g3d
