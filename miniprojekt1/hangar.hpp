#pragma once
// hangar.hpp - scena 3D: hangar, stanowiska, dron i efekty (klasyczny OpenGL 2.1 + GLU).

#include <array>

#include "g3d/camera.hpp"
#include "g3d/gl.hpp"
#include "mission.hpp"

namespace mp {

struct SceneResources {
    sf::Texture floor;
    bool hasFloor = false;
    GLUquadric* quadric = nullptr;
};

// Pozycje na ekranie (piksele, y w dol) potrzebne HUD do podpisania stanowisk w 3D.
struct ScreenAnchors {
    std::array<sf::Vector2f, STATION_COUNT> station;
    std::array<bool, STATION_COUNT> stationVisible;
};

void initSceneGL();
void resizeProjection(unsigned width, unsigned height);
bool loadSceneResources(SceneResources& r);      // wymaga aktywnego kontekstu okna
void freeSceneResources(SceneResources& r);      // przed zamknieciem okna

// Czysci bufory, ustawia kamere i swiatlo, rysuje cala scene. Zwraca punkty do podpisow HUD.
ScreenAnchors drawHangar(const SceneResources& r, const Mission& m, const g3d::OrbitCamera& camera);

} // namespace mp
