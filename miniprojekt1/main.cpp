/*
  GRAFIKA 3D - MINI-PROJEKT 1: DRON INSPEKCYJNY W HANGARZE 3D
  SFML 2.5.1 + OpenGL 2.1 (compatibility) + GLU, C++11 lub nowszy.
  Katalog roboczy: folder z assets/ (hangar_floor.png, fonts/DejaVuSans.ttf).

  Pliki:
    main.cpp      okno, petla programu, kamery, zdarzenia
    mission.*     stan misji: sterowanie dronem, kolizje AABB, inspekcja, restart
    hangar.*      scena 3D i efekty (klasyczny OpenGL + GLU)
    hud_view.*    HUD 2D przez SFML (pushGLStates/popGLStates)
    ../g3d/       wspolne pomocnicze naglowki z laboratoriow

  Sterowanie: patrz OPIS.md albo panel pomocy w programie (H).
*/
#include <cmath>
#include <iostream>

#include "g3d/camera.hpp"
#include "g3d/hud.hpp"
#include "g3d/screenshot.hpp"
#include "g3d/timer.hpp"
#include "hangar.hpp"
#include "hud_view.hpp"
#include "mission.hpp"

namespace {

// Dwie kamery orbitalne: "hangar" (jak w scenie startowej) i "poscig" za dronem.
struct CameraRig {
    g3d::OrbitCamera overview{8.8f, 0.6f, 0.57f, g3d::Vec3(0.0f, 0.18f, 0.0f)};
    g3d::OrbitCamera chase{4.3f, 0.0f, 0.42f, g3d::Vec3(0.0f, 0.72f, 0.0f)};
    bool chaseMode = true;
    float chaseOffset = 0.0f;      // obrot wokol drona ustawiony mysza (wzgledem jego nosa)

    CameraRig() {
        overview.minRadius = 5.2f;  overview.maxRadius = 15.0f;
        overview.minElevation = 0.10f; overview.maxElevation = 1.35f;
        overview.mouseSensitivity = 0.007f; overview.wheelStep = 0.55f;
        chase.minRadius = 2.2f;     chase.maxRadius = 9.0f;
        chase.minElevation = 0.05f; chase.maxElevation = 1.35f;
        chase.mouseSensitivity = 0.007f; chase.wheelStep = 0.45f;
    }
    g3d::OrbitCamera& active() { return chaseMode ? chase : overview; }

    void handleEvent(const sf::Event& e) {
        const float before = chase.azimuth;
        active().handleEvent(e);
        if (chaseMode) chaseOffset += chase.azimuth - before;   // przeciaganie zmienia offset
    }

    void snapToDrone(const mp::Drone& d) {
        chase.target = d.position;
        chase.azimuth = d.heading + chaseOffset;
    }

    // Kamera poscigowa plynnie podaza za polozeniem i obrotem drona.
    void update(const mp::Drone& d, float dt) {
        const float follow = 1.0f - std::exp(-5.0f * dt);
        chase.azimuth += g3d::angleDelta(chase.azimuth, d.heading + chaseOffset) * follow;
        chase.target.x = g3d::smoothTowards(chase.target.x, d.position.x, 8.0f, dt);
        chase.target.y = g3d::smoothTowards(chase.target.y, d.position.y, 8.0f, dt);
        chase.target.z = g3d::smoothTowards(chase.target.z, d.position.z, 8.0f, dt);
    }
};

} // namespace

int main() {
    const sf::ContextSettings settings(24, 0, 0, 2, 1);
    sf::RenderWindow window(sf::VideoMode(1152, 768), L"Mini-projekt 1 \u2014 Dron inspekcyjny",
                            sf::Style::Default, settings);
    window.setVerticalSyncEnabled(true);
    window.setKeyRepeatEnabled(false);
    window.setActive(true);

    mp::initSceneGL();
    mp::resizeProjection(window.getSize().x, window.getSize().y);
    mp::SceneResources resources;
    if (!mp::loadSceneResources(resources)) {
        std::cerr << "Nie mozna utworzyc kwadryki GLU.\n";
        return 1;
    }
    g3d::Hud hud;
    if (!hud.loadFont("assets/fonts/DejaVuSans.ttf"))
        std::cerr << "Brak assets/fonts/DejaVuSans.ttf - HUD bez napisow, stan misji w tytule okna.\n";

    mp::Mission mission;
    mp::resetMission(mission);
    CameraRig cameras;
    cameras.snapToDrone(mission.drone);
    mp::HudOptions hudOptions;
    bool screenshotRequested = false;
    g3d::FrameTimer timer(0.05f);
    sf::String lastTitle;

    std::cout << "W/S/A/D lot i obrot, Q/E bok, R/F gora/dol, Spacja (trzymaj) skan, Enter restart,\n"
              << "V kamera, LPM + mysz obrot kamery, kolko zoom, N reset kamery, P pauza, H pomoc, F12 zrzut, Esc.\n";

    while (window.isOpen()) {
        const float dt = timer.restart();
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) window.close();
            if (event.type == sf::Event::Resized) mp::resizeProjection(event.size.width, event.size.height);
            if (event.type == sf::Event::KeyPressed) {
                switch (event.key.code) {
                case sf::Keyboard::Escape: window.close(); break;
                case sf::Keyboard::Return:
                case sf::Keyboard::BackSpace:                        // restart bez zamykania programu
                    mp::resetMission(mission);
                    cameras.chaseOffset = 0.0f;
                    cameras.snapToDrone(mission.drone);
                    hudOptions.paused = false;
                    break;
                case sf::Keyboard::Space: if (!hudOptions.paused) mp::onScanPressed(mission); break;
                case sf::Keyboard::H: case sf::Keyboard::F1: hudOptions.showHelp = !hudOptions.showHelp; break;
                case sf::Keyboard::P: hudOptions.paused = !hudOptions.paused; break;
                case sf::Keyboard::V:
                    cameras.chaseMode = !cameras.chaseMode;
                    if (cameras.chaseMode) cameras.snapToDrone(mission.drone);
                    break;
                case sf::Keyboard::N:
                    cameras.overview.reset();
                    cameras.chase.reset();
                    cameras.chaseOffset = 0.0f;
                    cameras.snapToDrone(mission.drone);
                    break;
                case sf::Keyboard::F12: screenshotRequested = true; break;
                default: break;
                }
            }
            cameras.handleEvent(event);
        }
        if (!window.isOpen()) break;

        if (!hudOptions.paused) {
            // Bez fokusu okna klawiatura jest ignorowana (isKeyPressed dziala globalnie).
            const mp::InputState input = window.hasFocus() ? mp::readKeyboard() : mp::InputState();
            mp::updateMission(mission, input, dt);
            cameras.update(mission.drone, dt);
        }
        hudOptions.chaseCamera = cameras.chaseMode;
        hudOptions.viewAzimuth = cameras.active().azimuth;

        const sf::String title = mp::windowTitle(mission, hudOptions);
        if (title != lastTitle) { window.setTitle(title); lastTitle = title; }

        const mp::ScreenAnchors anchors = mp::drawHangar(resources, mission, cameras.active());
        hud.begin(window);
        mp::drawHud(hud, mission, anchors, hudOptions);
        hud.end(window);
        if (screenshotRequested) {
            const bool ok = g3d::saveScreenshot(window.getSize().x, window.getSize().y, "MiniProjekt1_capture.png");
            std::cout << (ok ? "Zapisano MiniProjekt1_capture.png\n" : "Blad zapisu zrzutu ekranu\n");
            screenshotRequested = false;
        }
        window.display();
    }
    // Zasoby OpenGL usuwamy PRZED zamknieciem kontekstu okna.
    mp::freeSceneResources(resources);
    return 0;
}
