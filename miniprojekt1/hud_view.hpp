#pragma once
// hud_view.hpp - czytelny stan misji na ekranie: panel stanowisk, minimapa, kompas, komunikaty.

#include "g3d/hud.hpp"
#include "hangar.hpp"
#include "mission.hpp"

namespace mp {

struct HudOptions {
    bool showHelp = true;
    bool paused = false;
    bool chaseCamera = true;
    float viewAzimuth = 0.0f;      // azymut aktywnej kamery - do kompasu
};

void drawHud(g3d::Hud& hud, const Mission& m, const ScreenAnchors& anchors, const HudOptions& o);
sf::String windowTitle(const Mission& m, const HudOptions& o);

} // namespace mp
