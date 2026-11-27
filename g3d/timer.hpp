#pragma once
// g3d/timer.hpp - deltaTime niezalezny od liczby klatek, z gornym ograniczeniem
// (po zminimalizowaniu okna albo zatrzymaniu w debuggerze nie bedzie wielkiego skoku).
//
//   g3d::FrameTimer timer;
//   while (window.isOpen()) { const float dt = timer.restart(); ... }

#include <algorithm>

#include <SFML/System/Clock.hpp>

namespace g3d {

class FrameTimer {
public:
    explicit FrameTimer(float maxDt = 0.05f) : maxDt_(maxDt) {}
    float restart() { return std::min(clock_.restart().asSeconds(), maxDt_); }
private:
    sf::Clock clock_;
    float maxDt_;
};

} // namespace g3d
