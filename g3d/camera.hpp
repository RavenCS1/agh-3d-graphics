#pragma once
// g3d/camera.hpp - kamera orbitalna we wspolrzednych sferycznych (lab 2, zad. 7 i lab 4, zad. 15).
//
//   g3d::OrbitCamera cam(3.9f, 0.59f, 0.39f);   // promien, azymut, elewacja (radiany)
//   cam.handleEvent(event);                      // LPM + ruch myszy, kolko myszy
//   cam.updateKeys(dt);                          // F/H azymut, T/G elewacja, O/P promien
//   cam.apply();                                 // glLoadIdentity + gluLookAt (raz na klatke)
//
// Pozycja oka wzgledem celu:
//   x = r cos(el) sin(az),  y = r sin(el),  z = r cos(el) cos(az)

#include "gl.hpp"
#include "math.hpp"

namespace g3d {

struct OrbitKeys {
    sf::Keyboard::Key azimuthMinus = sf::Keyboard::F;
    sf::Keyboard::Key azimuthPlus = sf::Keyboard::H;
    sf::Keyboard::Key elevationPlus = sf::Keyboard::T;
    sf::Keyboard::Key elevationMinus = sf::Keyboard::G;
    sf::Keyboard::Key radiusMinus = sf::Keyboard::O;
    sf::Keyboard::Key radiusPlus = sf::Keyboard::P;

    // Zestaw bez klawiatury (np. gdy klawisze sa potrzebne do czegos innego).
    static OrbitKeys none() {
        OrbitKeys k;
        k.azimuthMinus = k.azimuthPlus = k.elevationPlus = k.elevationMinus =
            k.radiusMinus = k.radiusPlus = sf::Keyboard::Unknown;
        return k;
    }
};

class OrbitCamera {
public:
    float radius, azimuth, elevation;
    Vec3 target;

    float minRadius = 2.5f, maxRadius = 10.0f;
    float minElevation = -1.4f, maxElevation = 1.4f;
    float angularSpeed = 1.2f;      // rad/s dla klawiszy
    float radialSpeed = 2.0f;       // jednostki/s dla klawiszy
    float mouseSensitivity = 0.006f;// rad na piksel przeciagania
    float wheelStep = 0.35f;        // jednostki na krok kolka

    OrbitCamera(float r = 3.9f, float az = 0.59f, float el = 0.39f, Vec3 tgt = Vec3())
        : radius(r), azimuth(az), elevation(el), target(tgt),
          initialRadius_(r), initialAzimuth_(az), initialElevation_(el), initialTarget_(tgt) {}

    // Polozenie oka w ukladzie swiata.
    Vec3 eye() const {
        return target + Vec3(radius * std::cos(elevation) * std::sin(azimuth),
                             radius * std::sin(elevation),
                             radius * std::cos(elevation) * std::cos(azimuth));
    }

    // Ustawia macierz widoku. Wywolaj raz na klatke, przed rysowaniem obiektow
    // i przed glLightfv(..., GL_POSITION, ...), aby swiatlo bylo w ukladzie swiata.
    void apply() const {
        const Vec3 e = eye();
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        gluLookAt(e.x, e.y, e.z, target.x, target.y, target.z, 0.0, 1.0, 0.0);
    }

    // Kierunki w plaszczyznie XZ - do sterowania obiektem "wzgledem kamery".
    Vec3 forwardXZ() const { return Vec3(-std::sin(azimuth), 0.0f, -std::cos(azimuth)); }
    Vec3 rightXZ() const { return Vec3(std::cos(azimuth), 0.0f, -std::sin(azimuth)); }

    void updateKeys(float dt, const OrbitKeys& keys = OrbitKeys()) {
        const float da = angularSpeed * dt, dr = radialSpeed * dt;
        if (pressed(keys.azimuthMinus)) azimuth -= da;
        if (pressed(keys.azimuthPlus)) azimuth += da;
        if (pressed(keys.elevationPlus)) elevation += da;
        if (pressed(keys.elevationMinus)) elevation -= da;
        if (pressed(keys.radiusMinus)) radius -= dr;
        if (pressed(keys.radiusPlus)) radius += dr;
        clampValues();
    }

    // Obsluga myszy: przeciaganie LPM obraca kamere, kolko przybliza/oddala.
    // Zwraca true, jesli zdarzenie zostalo wykorzystane.
    bool handleEvent(const sf::Event& e) {
        if (e.type == sf::Event::MouseButtonPressed && e.mouseButton.button == sf::Mouse::Left) {
            dragging_ = true;
            lastMouse_ = sf::Vector2i(e.mouseButton.x, e.mouseButton.y);
            return true;
        }
        if (e.type == sf::Event::MouseButtonReleased && e.mouseButton.button == sf::Mouse::Left) {
            dragging_ = false;
            return true;
        }
        if (e.type == sf::Event::MouseMoved && dragging_) {
            const int dx = e.mouseMove.x - lastMouse_.x;
            const int dy = e.mouseMove.y - lastMouse_.y;
            azimuth += mouseSensitivity * static_cast<float>(dx);
            elevation -= mouseSensitivity * static_cast<float>(dy);
            lastMouse_ = sf::Vector2i(e.mouseMove.x, e.mouseMove.y);
            clampValues();
            return true;
        }
        if (e.type == sf::Event::MouseWheelScrolled &&
            e.mouseWheelScroll.wheel == sf::Mouse::VerticalWheel) {
            radius -= wheelStep * e.mouseWheelScroll.delta;
            clampValues();
            return true;
        }
        if (e.type == sf::Event::LostFocus) dragging_ = false;
        return false;
    }

    void reset() {
        radius = initialRadius_;
        azimuth = initialAzimuth_;
        elevation = initialElevation_;
        target = initialTarget_;
    }

    void clampValues() {
        elevation = clampf(elevation, minElevation, maxElevation);
        radius = clampf(radius, minRadius, maxRadius);
    }

private:
    static bool pressed(sf::Keyboard::Key k) {
        return k != sf::Keyboard::Unknown && sf::Keyboard::isKeyPressed(k);
    }

    float initialRadius_, initialAzimuth_, initialElevation_;
    Vec3 initialTarget_;
    bool dragging_ = false;
    sf::Vector2i lastMouse_;
};

} // namespace g3d
