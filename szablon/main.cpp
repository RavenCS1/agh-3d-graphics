// ============================================================================
// Szablon na kolejne laboratoria: gotowa scena 3D zbudowana z mechanizmow g3d.
// Skopiuj ten plik i folder g3d/ do nowego projektu i rozbudowuj funkcje drawScene()/update().
//
//   LPM + mysz, kolko   kamera orbitalna;  F/H T/G O/P - to samo z klawiatury;  N reset
//   9 / 0, V / B        perspektywa / orto, fov
//   M                   oswietlenie wl./wyl.;   L  szkielet siatki;   K  normalne
//   Spacja              pauza animacji
//   F12                 zrzut ekranu (capture.png);   Esc wyjscie
// ============================================================================
#include <cmath>
#include <iostream>

#include "g3d/g3d.hpp"

namespace {

struct App {
    g3d::OrbitCamera camera{4.0f, 0.6f, 0.45f};
    g3d::Projection projection;
    g3d::GridMesh surface{21, 1.0f};      // siatka 21 x 21 na x, z w [-1, 1]
    float time = 0.0f;
    bool paused = false;
    bool lighting = true;
    bool wireframe = false;
    bool showNormals = false;
};

void initGL() {
    glClearColor(0.10f, 0.14f, 0.20f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glShadeModel(GL_SMOOTH);
    g3d::setupLight(GL_LIGHT0, g3d::Rgba(0.08f, 0.08f, 0.08f), g3d::Rgba(0.9f, 0.9f, 0.9f), g3d::Rgba(0.7f, 0.7f, 0.7f));
    g3d::useColorMaterial(true);
    g3d::setMaterial(g3d::Rgba(0.8f, 0.8f, 0.8f), g3d::Rgba(0.3f, 0.3f, 0.3f), 32.0f);
}

void update(App& app, float dt) {
    if (!app.paused) app.time += dt;
    const float phase = 1.5f * app.time;
    app.surface.build([phase](float x, float z) {
        return -0.35f + 0.25f * std::sin(g3d::PI * x - phase) * std::cos(g3d::PI * z + phase);
    });
}

void drawScene(const App& app) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    app.camera.apply();                                        // 1) kamera
    g3d::setLightPosition(GL_LIGHT0, g3d::Vec3(1.5f, 2.2f, 1.3f));   // 2) swiatlo w ukladzie swiata

    g3d::drawAxes(1.4f);                                       // osie bez oswietlenia

    {   // powierzchnia z oswietleniem
        g3d::ScopedCapability light(GL_LIGHTING, app.lighting);
        glPolygonMode(GL_FRONT_AND_BACK, app.wireframe ? GL_LINE : GL_FILL);
        glColor3f(0.15f, 0.70f, 0.78f);
        app.surface.draw(true);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    if (app.showNormals) {
        glColor3f(1.0f, 0.4f, 0.2f);
        app.surface.drawNormals(0.15f, 4);
    }
    {   // przyklad obiektu w transformacji T * R * S
        g3d::ScopedMatrix m;
        glTranslatef(0.0f, 0.6f, 0.0f);
        glRotatef(40.0f * app.time, 0.0f, 1.0f, 0.0f);
        g3d::drawWireCube(0.25f, 1.0f, 0.85f, 0.3f);
    }
}

} // namespace

int main() {
    const sf::ContextSettings settings(24, 0, 0, 2, 1);
    sf::RenderWindow window(sf::VideoMode(1000, 700), "Grafika 3D - szablon g3d", sf::Style::Default, settings);
    window.setVerticalSyncEnabled(true);
    window.setKeyRepeatEnabled(false);
    window.setActive(true);

    App app;
    initGL();
    app.projection.apply(window.getSize().x, window.getSize().y);
    g3d::FrameTimer timer;

    while (window.isOpen()) {
        const float dt = timer.restart();
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) window.close();
            if (event.type == sf::Event::Resized) app.projection.apply(event.size.width, event.size.height);
            if (event.type == sf::Event::KeyPressed) {
                const sf::Keyboard::Key key = event.key.code;
                if (key == sf::Keyboard::Escape) window.close();
                if (key == sf::Keyboard::N) app.camera.reset();
                if (key == sf::Keyboard::M) app.lighting = !app.lighting;
                if (key == sf::Keyboard::L) app.wireframe = !app.wireframe;
                if (key == sf::Keyboard::K) app.showNormals = !app.showNormals;
                if (key == sf::Keyboard::Space) app.paused = !app.paused;
                if (key == sf::Keyboard::F12) {
                    drawScene(app);   // swieza klatka w tylnym buforze
                    g3d::saveScreenshot(window.getSize().x, window.getSize().y, "capture.png");
                }
                if (app.projection.handleKey(key)) app.projection.apply(window.getSize().x, window.getSize().y);
            }
            app.camera.handleEvent(event);
        }
        if (!window.isOpen()) break;

        app.camera.updateKeys(dt);
        update(app, dt);
        drawScene(app);
        window.display();
    }
    return 0;
}
