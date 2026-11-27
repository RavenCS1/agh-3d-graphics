#pragma once
// g3d - male, naglowkowe narzedzia do laboratoriow z Grafiki 3D (SFML 2.5.1 + OpenGL 2.1/GLU).
// Wystarczy #include "g3d/g3d.hpp" i katalog repozytorium w sciezce naglowkow.
//
//   math.hpp        Vec3, cross, normalize, clampf, smoothTowards, angleDelta
//   gl_state.hpp    RAII: ScopedMatrix, ScopedCapability, ScopedLineWidth, ScopedTransparency
//   camera.hpp      OrbitCamera - kamera orbitalna (klawisze + mysz)
//   projection.hpp  Projection - perspektywa / orto, fov, zmiana rozmiaru okna
//   draw.hpp        osie, szkielet szescianu, prostopadloscian, okregi i pierscienie na podlodze
//   lighting.hpp    swiatla, tlumienie, materialy, GL_COLOR_MATERIAL
//   mesh.hpp        GridMesh - siatka trojkatow z normalnymi flat/smooth i UV
//   texture.hpp     wczytywanie tekstur SFML, ScopedTexture
//   shader.hpp      wczytywanie shaderow GLSL, ScopedShader
//   collision.hpp   Aabb, penetration, slideMove
//   hud.hpp         Hud - tekst i ksztalty 2D na wierzchu sceny 3D
//   screenshot.hpp  saveScreenshot - zapis klatki do PNG
//   timer.hpp       FrameTimer - deltaTime z ograniczeniem

#include "math.hpp"
#include "gl.hpp"
#include "gl_state.hpp"
#include "camera.hpp"
#include "projection.hpp"
#include "draw.hpp"
#include "lighting.hpp"
#include "mesh.hpp"
#include "texture.hpp"
#include "shader.hpp"
#include "collision.hpp"
#include "hud.hpp"
#include "screenshot.hpp"
#include "timer.hpp"
