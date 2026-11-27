#pragma once
// g3d/draw.hpp - podstawowe ksztalty rysowane w trybie natychmiastowym (glBegin/glEnd).

#include "gl_state.hpp"
#include "math.hpp"

namespace g3d {

// Osie XYZ (czerwona, zielona, niebieska) bez oswietlenia i tekstury.
// negativeDashed = true dorysowuje ujemne polosie linia przerywana (lab 1, rozszerzenie zad. 2).
inline void drawAxes(float length = 1.5f, float lineWidth = 2.0f, bool negativeDashed = false) {
    ScopedCapability noLight(GL_LIGHTING, false);
    ScopedCapability noTexture(GL_TEXTURE_2D, false);
    ScopedLineWidth width(lineWidth);

    glBegin(GL_LINES);
    glColor3f(1.0f, 0.2f, 0.2f); glVertex3f(0, 0, 0); glVertex3f(length, 0, 0);
    glColor3f(0.2f, 1.0f, 0.2f); glVertex3f(0, 0, 0); glVertex3f(0, length, 0);
    glColor3f(0.2f, 0.5f, 1.0f); glVertex3f(0, 0, 0); glVertex3f(0, 0, length);
    glEnd();

    if (negativeDashed) {
        ScopedCapability stipple(GL_LINE_STIPPLE, true);
        glLineStipple(2, 0x00FF);           // 8 pikseli kreski, 8 przerwy, kazdy bit x2
        glBegin(GL_LINES);
        glColor3f(1.0f, 0.2f, 0.2f); glVertex3f(0, 0, 0); glVertex3f(-length, 0, 0);
        glColor3f(0.2f, 1.0f, 0.2f); glVertex3f(0, 0, 0); glVertex3f(0, -length, 0);
        glColor3f(0.2f, 0.5f, 1.0f); glVertex3f(0, 0, 0); glVertex3f(0, 0, -length);
        glEnd();
    }
}

// Szkielet szescianu o srodku w (0,0,0): 8 wierzcholkow, 12 krawedzi.
inline void drawWireCube(float halfSize, float r, float g, float b, float lineWidth = 2.0f) {
    const float s = halfSize;
    const GLfloat v[8][3] = {
        {-s, -s, -s}, { s, -s, -s}, { s,  s, -s}, {-s,  s, -s},
        {-s, -s,  s}, { s, -s,  s}, { s,  s,  s}, {-s,  s,  s}};
    const int edges[12][2] = {
        {0, 1}, {1, 2}, {2, 3}, {3, 0},
        {4, 5}, {5, 6}, {6, 7}, {7, 4},
        {0, 4}, {1, 5}, {2, 6}, {3, 7}};
    ScopedLineWidth width(lineWidth);
    glColor3f(r, g, b);
    glBegin(GL_LINES);
    for (int i = 0; i < 12; ++i) {
        glVertex3fv(v[edges[i][0]]);
        glVertex3fv(v[edges[i][1]]);
    }
    glEnd();
}

// Prostopadloscian o srodku w (0,0,0) z normalnymi scian (do oswietlenia).
inline void drawBox(float w, float h, float d) {
    const float x = w * 0.5f, y = h * 0.5f, z = d * 0.5f;
    glBegin(GL_QUADS);
    glNormal3f(0, 0, 1);  glVertex3f(-x, -y, z);  glVertex3f(x, -y, z);  glVertex3f(x, y, z);  glVertex3f(-x, y, z);
    glNormal3f(0, 0, -1); glVertex3f(x, -y, -z);  glVertex3f(-x, -y, -z); glVertex3f(-x, y, -z); glVertex3f(x, y, -z);
    glNormal3f(1, 0, 0);  glVertex3f(x, -y, z);   glVertex3f(x, -y, -z); glVertex3f(x, y, -z);  glVertex3f(x, y, z);
    glNormal3f(-1, 0, 0); glVertex3f(-x, -y, -z); glVertex3f(-x, -y, z); glVertex3f(-x, y, z);  glVertex3f(-x, y, -z);
    glNormal3f(0, 1, 0);  glVertex3f(-x, y, z);   glVertex3f(x, y, z);   glVertex3f(x, y, -z);  glVertex3f(-x, y, -z);
    glNormal3f(0, -1, 0); glVertex3f(-x, -y, -z); glVertex3f(x, -y, -z); glVertex3f(x, -y, z);  glVertex3f(-x, -y, z);
    glEnd();
}

// Okrag lub luk w plaszczyznie poziomej (y = const). fraction = 1 to pelny okrag,
// 0.25 to cwiartka (np. pasek postepu na podlodze).
inline void drawArcXZ(float cx, float y, float cz, float radius, float fraction = 1.0f,
                      int segments = 64, float startAngle = 0.0f) {
    fraction = clampf(fraction, 0.0f, 1.0f);
    if (fraction <= 0.0f) return;
    const int n = std::max(2, static_cast<int>(static_cast<float>(segments) * fraction));
    glBegin(fraction >= 1.0f ? GL_LINE_LOOP : GL_LINE_STRIP);
    for (int i = 0; i <= n; ++i) {
        if (fraction >= 1.0f && i == n) break;
        const float t = startAngle + 2.0f * PI * fraction * static_cast<float>(i) / static_cast<float>(n);
        glVertex3f(cx + radius * std::sin(t), y, cz - radius * std::cos(t));
    }
    glEnd();
}

// Wypelniony pierscien (albo kolo, gdy inner = 0) w plaszczyznie poziomej.
inline void drawRingXZ(float cx, float y, float cz, float inner, float outer, int segments = 64) {
    glBegin(GL_TRIANGLE_STRIP);
    for (int i = 0; i <= segments; ++i) {
        const float t = 2.0f * PI * static_cast<float>(i) / static_cast<float>(segments);
        const float s = std::sin(t), c = std::cos(t);
        glNormal3f(0, 1, 0);
        glVertex3f(cx + outer * s, y, cz - outer * c);
        glVertex3f(cx + inner * s, y, cz - inner * c);
    }
    glEnd();
}

} // namespace g3d
