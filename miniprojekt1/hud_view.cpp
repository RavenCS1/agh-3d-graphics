// Plik zapisany w UTF-8 z BOM: polskie znaki w literalach L"..." dzialaja w MSVC
// takze bez przelacznika /utf-8.
#include "hud_view.hpp"

#include <algorithm>
#include <cmath>
#include <cwchar>
#include <string>

namespace mp {

namespace {

const sf::Color TEXT(235, 241, 248);
const sf::Color MUTED(158, 176, 198);
const sf::Color KEY(255, 214, 120);
const sf::Color CYAN(80, 200, 255);
const sf::Color AMBER(255, 186, 60);
const sf::Color GREEN(70, 238, 125);
const sf::Color RED(255, 92, 80);
const sf::Color PANEL(9, 17, 29, 200);
const sf::Color OUTLINE(90, 140, 180, 130);

const float MARGIN = 16.0f;
const float PANEL_W = 340.0f;
const float MAP_SIZE = 190.0f;

sf::Color withAlpha(sf::Color c, float a) {
    c.a = static_cast<sf::Uint8>(g3d::clampf(a, 0.0f, 1.0f) * c.a);
    return c;
}

std::wstring clockText(float seconds) {
    const int tenths = static_cast<int>(seconds * 10.0f);
    wchar_t buf[32];
    std::swprintf(buf, 32, L"%02d:%02d.%d", tenths / 600, (tenths % 600) / 10, tenths % 10);
    return buf;
}

std::wstring number(float value, int decimals) {
    wchar_t buf[32];
    std::swprintf(buf, 32, decimals == 0 ? L"%.0f" : L"%.1f", value);
    return buf;
}

std::wstring name(int station) {
    const char* label = STATIONS[static_cast<std::size_t>(station)].label;
    return std::wstring(label, label + std::char_traits<char>::length(label));
}

float measure(const g3d::Hud& hud, const sf::String& s, unsigned size, bool bold) {
    if (!hud.hasFont()) return 0.0f;
    sf::Text t(s, hud.font(), size);
    if (bold) t.setStyle(sf::Text::Bold);
    return t.getLocalBounds().width;
}

bool done(const Mission& m, int i) { return m.inspected[static_cast<std::size_t>(i)]; }

sf::Color stationColor(const Mission& m, int i) {
    if (done(m, i)) return GREEN;
    if (m.progressStation == i && m.scanProgress > 0.0f) return AMBER;
    if (m.zoneStation == i) return sf::Color(150, 230, 255);
    return CYAN;
}

std::wstring stationStatus(const Mission& m, int i) {
    if (done(m, i)) return L"sprawdzone ✓  " + clockText(m.inspectedClock[static_cast<std::size_t>(i)]);
    if (m.progressStation == i && m.scanProgress > 0.0f)
        return L"skanowanie " + number(m.scanProgress * 100.0f, 0) + L" %";
    if (m.zoneStation == i) return L"w zasięgu (Spacja)";
    return L"do sprawdzenia";
}

// ---------------------------------------------------------------------------
void drawMissionPanel(g3d::Hud& hud, const Mission& m) {
    const float x = MARGIN, y = MARGIN, w = PANEL_W, h = 222.0f;
    hud.panel(x, y, w, h, PANEL, OUTLINE);
    hud.text(L"INSPEKCJA HANGARU", x + 14, y + 10, 17, TEXT, 0, true);
    hud.text(clockText(m.missionTime), x + w - 14, y + 10, 17, m.finished ? GREEN : TEXT, 2, true);

    const int count = m.inspectedCount();
    hud.text(L"Sprawdzone stanowiska: " + number(static_cast<float>(count), 0) + L" / 3",
             x + 14, y + 38, 15, m.finished ? GREEN : MUTED);
    hud.bar(x + 14, y + 62, w - 28, 8, static_cast<float>(count) / STATION_COUNT, GREEN);

    for (int i = 0; i < STATION_COUNT; ++i) {
        const float rowY = y + 84.0f + 44.0f * static_cast<float>(i);
        const sf::Color color = stationColor(m, i);
        hud.circle(x + 24, rowY + 11, 7, color, done(m, i) ? sf::Color::Transparent : withAlpha(sf::Color::White, 0.35f));
        hud.text(name(i), x + 40, rowY, 16, TEXT, 0, true);
        hud.text(stationStatus(m, i), x + 118, rowY + 1, 14, color);
        if (!done(m, i))
            hud.text(number(distanceToBeacon(m, i), 1) + L" m", x + w - 14, rowY + 1, 14, MUTED, 2);
        if (!done(m, i) && m.progressStation == i && m.scanProgress > 0.0f)
            hud.bar(x + 40, rowY + 24, w - 54, 5, m.scanProgress, AMBER);
    }
}

// Minimapa z gory: -Z do gory, X w prawo.
void drawMinimap(g3d::Hud& hud, const Mission& m, const HudOptions& o) {
    const float x = hud.width() - MARGIN - MAP_SIZE, y = MARGIN;
    hud.panel(x, y, MAP_SIZE, MAP_SIZE + 24, PANEL, OUTLINE);
    hud.text(L"MAPA", x + 10, y + 5, 13, MUTED, 0, true);
    const float half = 3.8f, mapX = x + 10, mapY = y + 26, mapS = MAP_SIZE - 20;
    const float scale = mapS / (2.0f * half);
    auto toMap = [&](float wx, float wz) { return sf::Vector2f(mapX + (wx + half) * scale, mapY + (wz + half) * scale); };

    hud.panel(mapX, mapY, mapS, mapS, sf::Color(30, 45, 62, 160));
    hud.panel(mapX, mapY, mapS, 3, sf::Color(70, 110, 140));       // sciana z = -3.83
    hud.panel(mapX, mapY, 3, mapS, sf::Color(70, 110, 140));       // sciana x = -3.83
    const sf::Vector2f pad = toMap(0.0f, 0.0f);
    hud.circle(pad.x, pad.y, 0.55f * scale, sf::Color::Transparent, withAlpha(KEY, 0.6f), 1.0f);

    for (int i = 0; i < STATION_COUNT; ++i) {
        const Station& s = STATIONS[static_cast<std::size_t>(i)];
        const sf::Vector2f p = toMap(s.x, s.z);
        const sf::Color c = stationColor(m, i);
        if (!done(m, i)) hud.circle(p.x, p.y, tuning::SCAN_RADIUS * scale, withAlpha(c, 0.12f), withAlpha(c, 0.5f), 1.0f);
        hud.circle(p.x, p.y, 5, c);
        hud.text(name(i), p.x, p.y + 7, 11, c, 1);
    }
    const sf::Vector2f d = toMap(m.drone.position.x, m.drone.position.z);
    hud.arrow(d.x, d.y, 8, -m.drone.heading, m.drone.collisionFlash > 0.0f ? RED : sf::Color::White);

    hud.text(o.chaseCamera ? L"Kamera: pościg  (V)" : L"Kamera: hangar  (V)", x + MAP_SIZE * 0.5f,
             y + MAP_SIZE + 30, 13, MUTED, 1);
    hud.text(L"Wysokość: " + number(m.drone.position.y - FLOOR_Y, 1) + L" m", x + MAP_SIZE * 0.5f,
             y + MAP_SIZE + 48, 13, MUTED, 1);
}

// Podpisy nad stanowiskami (pozycje z gluProject).
void drawStationLabels(g3d::Hud& hud, const Mission& m, const ScreenAnchors& a) {
    for (int i = 0; i < STATION_COUNT; ++i) {
        const std::size_t idx = static_cast<std::size_t>(i);
        if (!a.stationVisible[idx]) continue;
        const std::wstring label = name(i) + (done(m, i) ? L" ✓" : L"");
        hud.text(label, a.station[idx].x, a.station[idx].y - 22, 15, stationColor(m, i), 1, true);
    }
}

// Kompas: kierunek do najblizszego niesprawdzonego stanowiska wzgledem widoku kamery.
void drawCompass(g3d::Hud& hud, const Mission& m, const HudOptions& o) {
    const int target = nearestPendingStation(m);
    if (target < 0 || m.zoneStation == target) return;
    const g3d::Vec3 d = beaconPosition(target) - m.drone.position;
    const g3d::Vec3 forward(-std::sin(o.viewAzimuth), 0.0f, -std::cos(o.viewAzimuth));
    const g3d::Vec3 right(std::cos(o.viewAzimuth), 0.0f, -std::sin(o.viewAzimuth));
    const float angle = std::atan2(g3d::dot(d, right), g3d::dot(d, forward));
    const float cx = hud.width() * 0.5f, cy = hud.height() - 128.0f;
    hud.circle(cx, cy, 24, PANEL, OUTLINE);
    hud.arrow(cx, cy, 14, angle, stationColor(m, target));
    hud.text(name(target) + L"  " + number(distanceToBeacon(m, target), 1) + L" m", cx + 34, cy - 10, 15, TEXT);
}

void drawHint(g3d::Hud& hud, const Mission& m) {
    if (m.finished) return;
    const float cx = hud.width() * 0.5f, y = hud.height() - 86.0f;
    std::wstring hint;
    sf::Color color = TEXT;
    const int zone = m.zoneStation;
    if (zone >= 0 && done(m, zone)) {
        hint = name(zone) + L" jest już sprawdzone — leć do następnego stanowiska";
        color = GREEN;
    } else if (zone >= 0) {
        hint = m.scanning ? L"Skanowanie " + name(zone) + L"…  " + number(m.scanProgress * 100.0f, 0) + L" %"
                          : L"Przytrzymaj SPACJĘ, aby skanować " + name(zone);
        color = m.scanning ? AMBER : sf::Color(150, 230, 255);
    } else {
        const int target = nearestPendingStation(m);
        hint = L"Leć do " + name(target) + L" i zbliż się do latarni na mniej niż " +
               number(tuning::SCAN_RADIUS, 1) + L" m";
        color = MUTED;
    }
    const float w = measure(hud, hint, 17, false) + 32.0f;
    hud.panel(cx - w * 0.5f, y - 6, w, 32, PANEL);
    hud.text(hint, cx, y, 17, color, 1);
    if (zone >= 0 && !done(m, zone)) hud.bar(cx - 180, y + 34, 360, 10, m.scanProgress, AMBER);
}

void drawToast(g3d::Hud& hud, const Mission& m) {
    const MissionEvent& e = m.event;
    const float duration = e.kind == EventKind::Started ? 3.5f : 2.6f;
    if (e.kind == EventKind::None || e.age > duration) return;
    std::wstring text;
    sf::Color color = TEXT;
    switch (e.kind) {
    case EventKind::Started: text = L"Nowa misja — sprawdź stanowiska ALFA, BETA i GAMMA"; color = CYAN; break;
    case EventKind::Inspected:
        text = L"Stanowisko " + name(e.station) + L" sprawdzone (" + number(static_cast<float>(m.inspectedCount()), 0) + L"/3)";
        color = GREEN;
        break;
    case EventKind::AlreadyInspected:
        text = name(e.station) + L" było już sprawdzone — drugi raz się nie liczy";
        color = AMBER;
        break;
    case EventKind::OutOfRange: text = L"Za daleko — podleć bliżej latarni stanowiska"; color = AMBER; break;
    case EventKind::Collision: text = L"Kolizja! Dron zatrzymał się na przeszkodzie"; color = RED; break;
    case EventKind::MissionComplete: text = L"Wszystkie stanowiska sprawdzone!"; color = GREEN; break;
    case EventKind::None: return;
    }
    const float fade = g3d::clampf((duration - e.age) / 0.6f, 0.0f, 1.0f);
    const float left = MARGIN + PANEL_W + 12.0f, right = hud.width() - MARGIN - MAP_SIZE - 12.0f;
    const float cx = (left + right) * 0.5f, w = measure(hud, text, 16, true) + 36.0f;
    hud.panel(cx - w * 0.5f, MARGIN, w, 36, withAlpha(PANEL, fade), withAlpha(color, 0.8f * fade), 1.5f);
    hud.text(text, cx, MARGIN + 8, 16, withAlpha(color, fade), 1, true);
}

void drawBanner(g3d::Hud& hud, const Mission& m) {
    if (!m.finished) return;
    const float appear = g3d::clampf((m.time - m.finishedAt) / 0.5f, 0.0f, 1.0f);
    // Srodek obszaru miedzy panelem misji a minimapa - nic nie jest zasloniete.
    const float left = MARGIN + PANEL_W + 12.0f, right = hud.width() - MARGIN - MAP_SIZE - 12.0f;
    const float w = std::min(540.0f, right - left), h = 150.0f;
    const float x = (left + right) * 0.5f - w * 0.5f, y = hud.height() * 0.30f - h * 0.5f;
    const float glow = 0.75f + 0.25f * std::sin((m.time - m.finishedAt) * 4.0f);
    hud.panel(x, y, w, h, withAlpha(sf::Color(6, 30, 18, 225), appear), withAlpha(GREEN, glow * appear), 3.0f);
    const float cx = x + w * 0.5f;
    hud.text(L"MISJA ZAKOŃCZONA", cx, y + 18, 34, withAlpha(GREEN, appear), 1, true);
    hud.text(L"Wszystkie 3 stanowiska sprawdzone — czas " + clockText(m.missionTime), cx, y + 72, 18,
             withAlpha(TEXT, appear), 1);
    hud.text(L"Enter — nowa misja     Esc — wyjście", cx, y + 106, 16, withAlpha(MUTED, appear), 1);
}

void drawHelp(g3d::Hud& hud, const HudOptions& o) {
    if (!o.showHelp) {
        hud.text(L"H — pomoc", MARGIN, hud.height() - 30, 14, MUTED);
        return;
    }
    static const wchar_t* const lines[][2] = {
        {L"W / S   (↑ / ↓)", L"lot do przodu / do tyłu"},
        {L"A / D   (← / →)", L"obrót drona"},
        {L"Q / E", L"lot w bok"},
        {L"R / F   (PgUp / PgDn)", L"wznoszenie / opadanie"},
        {L"Spacja (trzymaj)", L"skanowanie stanowiska"},
        {L"Enter / Backspace", L"restart misji"},
        {L"V", L"kamera: pościg / hangar"},
        {L"LPM + mysz, kółko", L"obrót i zoom kamery"},
        {L"N   /   P", L"reset kamery   /   pauza"},
        {L"F12   /   H", L"zrzut ekranu   /   ukryj pomoc"},
    };
    const int count = static_cast<int>(sizeof(lines) / sizeof(lines[0]));
    const float x = MARGIN, y = MARGIN + 222.0f + 12.0f, w = PANEL_W, h = 34.0f + 21.0f * count;
    hud.panel(x, y, w, h, PANEL, OUTLINE);
    hud.text(L"STEROWANIE", x + 14, y + 8, 14, MUTED, 0, true);
    for (int i = 0; i < count; ++i) {
        const float rowY = y + 30.0f + 21.0f * static_cast<float>(i);
        hud.text(lines[i][0], x + 14, rowY, 14, KEY);
        hud.text(lines[i][1], x + 172, rowY, 14, TEXT);
    }
}

void drawPause(g3d::Hud& hud) {
    hud.panel(0, 0, hud.width(), hud.height(), sf::Color(0, 0, 0, 120));
    hud.text(L"PAUZA", hud.width() * 0.5f, hud.height() * 0.5f - 40, 42, TEXT, 1, true);
    hud.text(L"P — wznów", hud.width() * 0.5f, hud.height() * 0.5f + 14, 18, MUTED, 1);
}

} // namespace

void drawHud(g3d::Hud& hud, const Mission& m, const ScreenAnchors& anchors, const HudOptions& o) {
    if (m.drone.collisionFlash > 0.0f) {           // czerwona ramka przy kolizji
        const float a = m.drone.collisionFlash / 0.35f;
        hud.panel(5, 5, hud.width() - 10, hud.height() - 10, sf::Color::Transparent, withAlpha(RED, a), 5.0f);
    }
    drawStationLabels(hud, m, anchors);
    drawMissionPanel(hud, m);
    drawMinimap(hud, m, o);
    drawHelp(hud, o);
    drawCompass(hud, m, o);
    drawHint(hud, m);
    drawToast(hud, m);
    drawBanner(hud, m);
    if (o.paused) drawPause(hud);
}

sf::String windowTitle(const Mission& m, const HudOptions& o) {
    std::wstring t = L"Mini-projekt 1 — Dron inspekcyjny  |  sprawdzone " +
                     number(static_cast<float>(m.inspectedCount()), 0) + L"/3  |  czas " + clockText(m.missionTime);
    if (m.finished) t += L"  |  MISJA ZAKOŃCZONA (Enter = nowa)";
    if (o.paused) t += L"  |  PAUZA";
    return t;
}

} // namespace mp
