#include "hangar.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>

#include "g3d/draw.hpp"
#include "g3d/gl_state.hpp"
#include "g3d/texture.hpp"

namespace mp {

namespace {

using g3d::Vec3;
using g3d::PI;

float mixf(float a, float b, float t) { return a + (b - a) * t; }

void setEmission(float r, float g, float b) {
    const GLfloat e[4] = {r, g, b, 1.0f};
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, e);
}

// Swiatlo ustawiamy PO kamerze: jego pozycja jest wyrazona w ukladzie swiata.
void applyLight() {
    const GLfloat position[] = {-1.6f, 5.3f, 2.3f, 1.0f};
    glLightfv(GL_LIGHT0, GL_POSITION, position);
}

// ---------------------------------------------------------------------------
// Hangar (scena startowa + ladowisko startowe)
// ---------------------------------------------------------------------------
void drawFloor(const SceneResources& r) {
    glColor3f(0.82f, 0.85f, 0.91f);
    {
        g3d::ScopedTexture texture(r.hasFloor ? &r.floor : nullptr);
        if (!r.hasFloor) glDisable(GL_TEXTURE_2D);
        glNormal3f(0, 1, 0);
        glBegin(GL_QUADS);
        glTexCoord2f(0, 0); glVertex3f(-3.8f, FLOOR_Y, -3.8f);
        glTexCoord2f(5, 0); glVertex3f(-3.8f, FLOOR_Y, 3.8f);
        glTexCoord2f(5, 5); glVertex3f(3.8f, FLOOR_Y, 3.8f);
        glTexCoord2f(0, 5); glVertex3f(3.8f, FLOOR_Y, -3.8f);
        glEnd();
    }
    // Sciany i pasy na podlodze.
    glColor3f(0.15f, 0.23f, 0.32f);
    { g3d::ScopedMatrix s; glTranslatef(0, 0.25f, -3.83f); g3d::drawBox(7.7f, 1.1f, 0.10f); }
    { g3d::ScopedMatrix s; glTranslatef(-3.83f, 0.25f, 0); g3d::drawBox(0.10f, 1.1f, 7.7f); }
    glColor3f(0.19f, 0.33f, 0.43f);
    for (int i = -3; i <= 3; ++i) {
        g3d::ScopedMatrix s;
        glTranslatef(static_cast<float>(i), FLOOR_Y + 0.025f, 0);
        g3d::drawBox(0.035f, 0.035f, 7.2f);
    }
}

void drawStartPad() {
    g3d::ScopedCapability noLight(GL_LIGHTING, false);
    g3d::ScopedLineWidth width(2.0f);
    const float y = FLOOR_Y + 0.05f;
    glColor3f(0.95f, 0.75f, 0.20f);
    g3d::drawArcXZ(0.0f, y, 0.0f, 0.55f);
    glBegin(GL_LINES);                               // litera H - punkt startu i restartu
    glVertex3f(-0.18f, y, -0.22f); glVertex3f(-0.18f, y, 0.22f);
    glVertex3f(0.18f, y, -0.22f);  glVertex3f(0.18f, y, 0.22f);
    glVertex3f(-0.18f, y, 0.0f);   glVertex3f(0.18f, y, 0.0f);
    glEnd();
}

// ---------------------------------------------------------------------------
// Stanowiska: kolor latarni i pierscieni zalezy od stanu inspekcji
// ---------------------------------------------------------------------------
void beaconColor(const Mission& m, int i, float rgb[3], float& glow) {
    const std::size_t idx = static_cast<std::size_t>(i);
    const float pulse = 0.82f + 0.18f * std::sin(m.time * 3.3f + static_cast<float>(i));
    if (m.inspected[idx]) {
        // Po ukonczeniu misji wszystkie latarnie pulsuja razem.
        const float beat = m.finished ? 0.80f + 0.20f * std::sin((m.time - m.finishedAt) * 6.0f) : 1.0f;
        rgb[0] = 0.14f * beat; rgb[1] = 0.98f * beat; rgb[2] = 0.38f * beat;
        glow = 0.35f * beat;
    } else if (m.scanning && m.progressStation == i) {
        const float t = m.scanProgress;
        const float flicker = 0.80f + 0.20f * std::sin(m.time * 18.0f);
        rgb[0] = mixf(0.18f, 1.00f, t) * flicker;
        rgb[1] = mixf(0.79f, 0.72f, t) * flicker;
        rgb[2] = mixf(1.00f, 0.15f, t) * flicker;
        glow = 0.25f + 0.35f * t;
    } else if (m.zoneStation == i) {                  // w zasiegu, czeka na Spacje
        const float fast = 0.75f + 0.25f * std::sin(m.time * 8.0f);
        rgb[0] = 0.30f * fast; rgb[1] = 0.90f * fast; rgb[2] = 1.0f * fast;
        glow = 0.22f;
    } else {                                          // kolor ze sceny startowej
        rgb[0] = 0.18f * pulse; rgb[1] = 0.79f * pulse; rgb[2] = 1.0f * pulse;
        glow = 0.08f;
    }
}

void drawStation(const SceneResources& r, const Mission& m, int i) {
    const Station& s = STATIONS[static_cast<std::size_t>(i)];
    const bool done = m.inspected[static_cast<std::size_t>(i)];
    g3d::ScopedMatrix station;
    glTranslatef(s.x, 0.0f, s.z);

    glColor3f(0.20f, 0.27f, 0.35f);
    { g3d::ScopedMatrix p; glTranslatef(0, -0.1f, 0); g3d::drawBox(0.9f, 0.3f, 0.9f); }
    glColor3f(0.31f, 0.42f, 0.52f);
    { g3d::ScopedMatrix p; glTranslatef(0, 0.22f, 0); g3d::drawBox(0.30f, 0.65f, 0.30f); }

    float rgb[3], glow;
    beaconColor(m, i, rgb, glow);
    glColor3f(rgb[0], rgb[1], rgb[2]);
    setEmission(rgb[0] * glow, rgb[1] * glow, rgb[2] * glow);   // latarnia "swieci"
    { g3d::ScopedMatrix p; glTranslatef(0, tuning::BEACON_Y, 0); gluSphere(r.quadric, 0.22, 20, 14); }
    setEmission(0.0f, 0.0f, 0.0f);

    // Ramki na podlodze - bez oswietlenia. Zielona podwojna ramka = sprawdzone.
    g3d::ScopedCapability noLight(GL_LIGHTING, false);
    g3d::ScopedLineWidth width(done ? 2.5f : 1.5f);
    if (done) glColor3f(0.20f, 1.0f, 0.45f);
    else glColor3f(rgb[0], rgb[1], rgb[2]);
    g3d::drawArcXZ(0.0f, FLOOR_Y + 0.02f, 0.0f, 0.44f, 1.0f, 40);
    if (done) g3d::drawArcXZ(0.0f, FLOOR_Y + 0.02f, 0.0f, 0.58f, 1.0f, 48);
    if (!done && m.progressStation == i && m.scanProgress > 0.0f) {
        g3d::ScopedLineWidth thick(5.0f);             // postep skanowania jako luk na podlodze
        glColor3f(1.0f, 0.72f, 0.15f);
        g3d::drawArcXZ(0.0f, FLOOR_Y + 0.03f, 0.0f, 0.58f, m.scanProgress, 64);
    }
}

// ---------------------------------------------------------------------------
// Dron: geometria ze sceny startowej + pochylenie, kolizja, swiatelko, rozmyte wirniki
// ---------------------------------------------------------------------------
void applyDroneTransform(const Mission& m) {
    const Drone& d = m.drone;
    const float bob = 0.025f * std::sin(m.time * 2.4f);        // unoszenie sie w zawisie
    glTranslatef(d.position.x, d.position.y + bob, d.position.z);
    glRotatef(g3d::radToDeg(d.heading), 0, 1, 0);
    glRotatef(-d.tiltForward, 1, 0, 0);                         // nos w dol przy locie do przodu
    glRotatef(-d.tiltRight, 0, 0, 1);                           // przechyl na boki
}

void drawDrone(const SceneResources& r, const Mission& m) {
    const Drone& d = m.drone;
    const float hit = d.collisionFlash / 0.35f;                 // 1 -> 0 po uderzeniu
    g3d::ScopedMatrix drone;
    applyDroneTransform(m);

    glColor3f(mixf(0.86f, 1.0f, hit), mixf(0.87f, 0.25f, hit), mixf(0.91f, 0.22f, hit));
    { g3d::ScopedMatrix p; glScalef(1.20f, 0.40f, 0.80f); gluSphere(r.quadric, 0.30, 24, 16); }
    glColor3f(0.31f, 0.73f, 0.90f);                              // "kamera" z przodu = kierunek lotu
    { g3d::ScopedMatrix p; glTranslatef(0, 0.04f, -0.34f); gluSphere(r.quadric, 0.11, 14, 10); }
    glColor3f(0.45f, 0.51f, 0.58f);
    g3d::drawBox(1.14f, 0.065f, 0.07f);
    g3d::drawBox(0.07f, 0.065f, 0.90f);

    // Migajace swiatlo na grzbiecie (1 Hz).
    const bool blink = std::fmod(m.time, 1.0f) < 0.18f;
    glColor3f(blink ? 1.0f : 0.35f, 0.15f, 0.12f);
    if (blink) setEmission(0.9f, 0.1f, 0.05f);
    { g3d::ScopedMatrix p; glTranslatef(0, 0.12f, 0.05f); gluSphere(r.quadric, 0.045, 10, 6); }
    setEmission(0.0f, 0.0f, 0.0f);

    for (int sx = -1; sx <= 1; sx += 2) {
        for (int sz = -1; sz <= 1; sz += 2) {
            g3d::ScopedMatrix rotor;
            glTranslatef(static_cast<float>(sx) * 0.56f, 0.05f, static_cast<float>(sz) * 0.45f);
            glColor3f(0.33f, 0.40f, 0.50f);
            gluSphere(r.quadric, 0.10, 12, 8);
            g3d::ScopedCapability noLight(GL_LIGHTING, false);
            glColor3f(0.14f, 0.90f, 1.0f);
            glRotatef(d.rotorAngle * static_cast<float>(sx * sz), 0, 1, 0);   // sasiednie wirniki w przeciwne strony
            glBegin(GL_LINES);
            glVertex3f(-0.18f, 0.06f, 0); glVertex3f(0.18f, 0.06f, 0);
            glVertex3f(0, 0.06f, -0.18f); glVertex3f(0, 0.06f, 0.18f);
            glEnd();
        }
    }
}

// Polprzezroczyste tarcze wirnikow - im szybciej sie kreca, tym wyrazniejsze.
void drawRotorBlur(const Mission& m) {
    const float strength = g3d::clampf((m.drone.rotorSpeed - 800.0f) / 1400.0f, 0.0f, 1.0f);
    g3d::ScopedMatrix drone;
    applyDroneTransform(m);
    glColor4f(0.55f, 0.95f, 1.0f, 0.10f + 0.16f * strength);
    for (int sx = -1; sx <= 1; sx += 2)
        for (int sz = -1; sz <= 1; sz += 2) g3d::drawRingXZ(static_cast<float>(sx) * 0.56f, 0.115f, static_cast<float>(sz) * 0.45f, 0.0f, 0.19f, 24);
}

// Miekki cien pod dronem - ulatwia ocene wysokosci i polozenia nad podloga.
void drawDroneShadow(const Mission& m) {
    const Drone& d = m.drone;
    const float height = d.position.y - FLOOR_Y;
    const float radius = 0.45f + 0.12f * height;
    const float alpha = g3d::clampf(0.75f - 0.18f * height, 0.25f, 0.70f);
    const float y = FLOOR_Y + 0.006f;
    glBegin(GL_TRIANGLE_FAN);
    glColor4f(0.0f, 0.0f, 0.0f, alpha);
    glVertex3f(d.position.x, y, d.position.z);
    glColor4f(0.0f, 0.0f, 0.0f, 0.0f);
    for (int i = 0; i <= 32; ++i) {
        const float t = 2.0f * PI * static_cast<float>(i) / 32.0f;
        glVertex3f(d.position.x + radius * std::sin(t), y, d.position.z - radius * std::cos(t));
    }
    glEnd();
}

// Holograficzna kula pokazujaca strefe inspekcji, widoczna gdy dron sie zbliza.
void drawScanZones(const SceneResources& r, const Mission& m) {
    for (int i = 0; i < STATION_COUNT; ++i) {
        if (m.inspected[static_cast<std::size_t>(i)]) continue;
        const float dist = distanceToBeacon(m, i);
        const float closeness = g3d::clampf(1.0f - (dist - tuning::SCAN_RADIUS) / 1.6f, 0.0f, 1.0f);
        if (closeness <= 0.0f) continue;
        const Vec3 b = beaconPosition(i);
        g3d::ScopedMatrix p;
        glTranslatef(b.x, b.y, b.z);
        glRotatef(m.time * 20.0f, 0, 1, 0);
        if (m.zoneStation == i) glColor4f(1.0f, 0.80f, 0.25f, 0.30f);
        else glColor4f(0.30f, 0.85f, 1.0f, 0.22f * closeness);
        gluQuadricDrawStyle(r.quadric, GLU_LINE);
        gluSphere(r.quadric, tuning::SCAN_RADIUS, 18, 10);
        gluQuadricDrawStyle(r.quadric, GLU_FILL);
    }
}

// Stozek skanera od drona do latarni, migoczacy w czasie skanowania.
void drawScanBeam(const Mission& m) {
    if (!m.scanning || m.progressStation < 0) return;
    const Vec3 apex = m.drone.position - Vec3(0.0f, 0.10f, 0.0f);
    const Vec3 target = beaconPosition(m.progressStation);
    const Vec3 dir = g3d::normalize(target - apex);
    const Vec3 helper = std::fabs(dir.y) > 0.9f ? Vec3(1, 0, 0) : Vec3(0, 1, 0);
    const Vec3 u = g3d::normalize(g3d::cross(dir, helper));
    const Vec3 v = g3d::cross(dir, u);
    const float flicker = 0.75f + 0.25f * std::sin(m.time * 30.0f);
    const float radius = 0.26f;

    glBegin(GL_TRIANGLE_FAN);
    glColor4f(1.0f, 0.85f, 0.35f, 0.55f * flicker);
    glVertex3f(apex.x, apex.y, apex.z);
    glColor4f(1.0f, 0.70f, 0.15f, 0.10f);
    for (int i = 0; i <= 24; ++i) {
        const float t = 2.0f * PI * static_cast<float>(i) / 24.0f;
        const Vec3 p = target + u * (radius * std::cos(t)) + v * (radius * std::sin(t));
        glVertex3f(p.x, p.y, p.z);
    }
    glEnd();
    g3d::ScopedLineWidth width(2.0f);
    glBegin(GL_LINES);
    glColor4f(1.0f, 0.95f, 0.70f, 0.9f * flicker);
    glVertex3f(apex.x, apex.y, apex.z);
    glVertex3f(target.x, target.y, target.z);
    glEnd();
}

// Fala rozchodzaca sie od stanowiska po udanej inspekcji (1.4 s).
void drawShockwaves(const Mission& m) {
    const float duration = 1.4f;
    for (int i = 0; i < STATION_COUNT; ++i) {
        const std::size_t idx = static_cast<std::size_t>(i);
        if (!m.inspected[idx]) continue;
        const float age = m.time - m.inspectedAt[idx];
        if (age < 0.0f || age > duration) continue;
        const float k = age / duration;
        const float radius = 0.3f + 1.9f * k;
        const Vec3 b = beaconPosition(i);
        glColor4f(0.25f, 1.0f, 0.50f, 0.75f * (1.0f - k));
        g3d::drawRingXZ(b.x, b.y, b.z, radius - 0.07f, radius, 48);
        g3d::drawRingXZ(b.x, FLOOR_Y + 0.01f, b.z, radius * 1.2f - 0.05f, radius * 1.2f, 48);
    }
}

// Po ukonczeniu misji: slupy swiatla nad wszystkimi stanowiskami.
void drawVictoryPillars(const SceneResources& r, const Mission& m) {
    if (!m.finished) return;
    const float grow = g3d::clampf((m.time - m.finishedAt) / 0.8f, 0.0f, 1.0f);
    for (int i = 0; i < STATION_COUNT; ++i) {
        const Vec3 b = beaconPosition(i);
        const float alpha = (0.18f + 0.10f * std::sin(m.time * 3.0f + static_cast<float>(i))) * grow;
        g3d::ScopedMatrix p;
        glTranslatef(b.x, b.y + 0.2f, b.z);
        glRotatef(-90.0f, 1, 0, 0);
        glColor4f(0.30f, 1.0f, 0.55f, alpha);
        gluCylinder(r.quadric, 0.16, 0.34, 2.6 * grow, 20, 1);
    }
}

void computeAnchors(ScreenAnchors& a) {
    GLdouble model[16], projection[16];
    GLint viewport[4];
    glGetDoublev(GL_MODELVIEW_MATRIX, model);
    glGetDoublev(GL_PROJECTION_MATRIX, projection);
    glGetIntegerv(GL_VIEWPORT, viewport);
    for (int i = 0; i < STATION_COUNT; ++i) {
        const Vec3 p = beaconPosition(i) + Vec3(0.0f, 0.42f, 0.0f);
        GLdouble wx = 0, wy = 0, wz = 0;
        const bool ok = gluProject(p.x, p.y, p.z, model, projection, viewport, &wx, &wy, &wz) == GL_TRUE;
        const std::size_t idx = static_cast<std::size_t>(i);
        a.stationVisible[idx] = ok && wz > 0.0 && wz < 1.0;
        // OpenGL liczy y od dolu okna, SFML od gory.
        a.station[idx] = sf::Vector2f(static_cast<float>(wx), static_cast<float>(viewport[3] - wy));
    }
}

} // namespace

void initSceneGL() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_NORMALIZE);                       // prawidlowe normalne po glScalef
    glShadeModel(GL_SMOOTH);
    const GLfloat ambient[] = {0.19f, 0.24f, 0.30f, 1.0f};
    const GLfloat diffuse[] = {0.91f, 0.93f, 1.0f, 1.0f};
    const GLfloat specular[] = {0.55f, 0.65f, 0.80f, 1.0f};
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, specular);
    const GLfloat matSpec[] = {0.28f, 0.34f, 0.42f, 1.0f};
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, matSpec);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 38.0f);
}

void resizeProjection(unsigned width, unsigned height) {
    width = std::max(1u, width);
    height = std::max(1u, height);
    glViewport(0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height));
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(48.0, static_cast<double>(width) / height, 0.1, 80.0);
    glMatrixMode(GL_MODELVIEW);
}

bool loadSceneResources(SceneResources& r) {
    r.quadric = gluNewQuadric();
    if (!r.quadric) return false;
    gluQuadricNormals(r.quadric, GLU_SMOOTH);
    r.hasFloor = g3d::loadTexture(r.floor, "assets/hangar_floor.png", true, true);
    if (!r.hasFloor) std::cerr << "Podloga bedzie jednolita (brak assets/hangar_floor.png).\n";
    return true;
}

void freeSceneResources(SceneResources& r) {
    if (r.quadric) gluDeleteQuadric(r.quadric);
    r.quadric = nullptr;
}

ScreenAnchors drawHangar(const SceneResources& r, const Mission& m, const g3d::OrbitCamera& camera) {
    // Tlo lekko zielenieje po ukonczeniu misji.
    const float win = m.finished ? g3d::clampf((m.time - m.finishedAt) / 1.5f, 0.0f, 1.0f) : 0.0f;
    glClearColor(mixf(0.055f, 0.045f, win), mixf(0.080f, 0.130f, win), mixf(0.12f, 0.095f, win), 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    camera.apply();
    applyLight();
    ScreenAnchors anchors;
    computeAnchors(anchors);

    // 1) obiekty nieprzezroczyste
    drawFloor(r);
    drawStartPad();
    for (int i = 0; i < STATION_COUNT; ++i) drawStation(r, m, i);
    drawDrone(r, m);

    // 2) obiekty polprzezroczyste: GL_BLEND, bez zapisu glebokosci, test glebokosci aktywny
    {
        g3d::ScopedTransparency transparent;
        drawDroneShadow(m);
        drawShockwaves(m);
        drawRotorBlur(m);
        drawScanZones(r, m);
        drawScanBeam(m);
        drawVictoryPillars(r, m);
    }
    return anchors;
}

} // namespace mp
