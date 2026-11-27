#pragma once
// g3d/gl.hpp - naglowki SFML i klasycznego OpenGL/GLU w poprawnej kolejnosci.
// Na Windows SFML/OpenGL.hpp dolacza windows.h, ktory jest wymagany przed GL/glu.h.

#if defined(_WIN32) && !defined(NOMINMAX)
#define NOMINMAX   // windows.h (dolaczany przez SFML/OpenGL.hpp w MSVC) bez makr min/max
#endif

#include <SFML/Graphics.hpp>
#include <SFML/OpenGL.hpp>
#include <GL/glu.h>
