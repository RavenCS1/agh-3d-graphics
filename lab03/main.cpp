// ============================================================================
// Grafika 3D - Laboratorium 3: bryly GLU, oswietlenie i materialy
// Zadania 9-12 w jednym programie. C++11+, SFML 2.5.1, OpenGL 2.1 + GLU.
//
// Sterowanie (kamera i projekcja z laboratorium 2 dzialaja we wszystkich zadaniach):
//   F1/F2/F3/F4   stan po zadaniu 9/10/11/12 (start: zadanie 12)
//   F/H T/G O/P   kamera: azymut, elewacja, promien;  N reset kamery;  kolko myszy zoom
//   9 / 0, V / B  perspektywa / orto, fov
//   U             sfera 24 / 8 wycinkow (zad. 9)
//   L             GLU_FILL / GLU_LINE (zad. 9)
//   I             denka walca z gluDisk (rozszerzenie zad. 9)
//   M             oswietlenie bryl wl./wyl. (zad. 10+)
//   C             normalne kwadryki GLU_SMOOTH / GLU_NONE (zad. 10+)
//   A/D S/W Q/E   ruch swiatla w X / Y / Z (zad. 11+), Spacja - reset swiatla
//   Y             swiatlo punktowe / kierunkowe (rozszerzenie zad. 11)
//   Z / X         polysk prawej sfery -/+ 8 (zad. 12)
//   K             skladowa zwierciadlana prawej sfery wl./wyl. (zad. 12)
//   Esc           wyjscie
// ============================================================================

#if defined(_WIN32) && !defined(NOMINMAX)
#define NOMINMAX   // windows.h (dolaczany przez SFML/OpenGL.hpp w MSVC) bez makr min/max
#endif
#include <SFML/Graphics.hpp>
#include <SFML/OpenGL.hpp>
#include <GL/glu.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>

namespace {

float clampf(float v, float lo, float hi) { return std::max(lo, std::min(v, hi)); }

const sf::Vector3f LIGHT_START(1.7f, 2.0f, 1.7f);

struct OrbitCamera {
    float radius = 3.9f, azimuth = 0.59f, elevation = 0.39f;
};

struct AppState {
    int task = 12;
    OrbitCamera camera;
    bool perspective = true;
    float fov = 45.0f;
    // zad. 9
    bool lowResSphere = false;
    bool wireframe = false;
    bool cylinderCaps = true;
    // zad. 10
    bool lighting = true;
    bool smoothNormals = true;
    // zad. 11
    sf::Vector3f lightPosition = LIGHT_START;
    bool pointLight = true;
    // zad. 12
    float rightShininess = 96.0f;
    bool rightSpecular = true;
};

// ---------------------------------------------------------------------------
// Kamera i projekcja (laboratorium 2)
// ---------------------------------------------------------------------------
void setCamera(const OrbitCamera& c) {
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(c.radius * std::cos(c.elevation) * std::sin(c.azimuth),
              c.radius * std::sin(c.elevation),
              c.radius * std::cos(c.elevation) * std::cos(c.azimuth),
              0.0, 0.0, 0.0,  0.0, 1.0, 0.0);
}

void configureProjection(unsigned width, unsigned height, const AppState& s) {
    const unsigned w = std::max(1u, width), h = std::max(1u, height);
    const double aspect = static_cast<double>(w) / static_cast<double>(h);
    glViewport(0, 0, static_cast<GLsizei>(w), static_cast<GLsizei>(h));
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    if (s.perspective) gluPerspective(s.fov, aspect, 0.1, 100.0);
    else glOrtho(-1.75 * aspect, 1.75 * aspect, -1.75, 1.75, 0.1, 100.0);
    glMatrixMode(GL_MODELVIEW);
}

void drawAxes() {
    glDisable(GL_LIGHTING);                       // osie zawsze w czystych kolorach
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    glColor3f(1.0f, 0.2f, 0.2f); glVertex3f(0, 0, 0); glVertex3f(1.5f, 0, 0);
    glColor3f(0.2f, 1.0f, 0.2f); glVertex3f(0, 0, 0); glVertex3f(0, 1.5f, 0);
    glColor3f(0.2f, 0.5f, 1.0f); glVertex3f(0, 0, 0); glVertex3f(0, 0, 1.5f);
    glEnd();
    glLineWidth(1.0f);
}

// ============================================================================
// ZADANIE 9 - sfera, walec i stozek z jednej kwadryki GLU
// gluCylinder buduje powierzchnie wzdluz lokalnej osi +Z (od z=0 do z=height).
// Obrot o -90 stopni wokol X kieruje ja wzdluz swiatowej osi +Y.
// ============================================================================
void drawSphere(GLUquadric* q, int slices) {
    glPushMatrix();
    glTranslatef(-0.9f, 0.0f, 0.0f);
    gluSphere(q, 0.35, slices, 16);
    glPopMatrix();
}

void drawCylinder(GLUquadric* q, bool caps) {
    glPushMatrix();
    glTranslatef(0.0f, -0.45f, 0.0f);             // srodek walca w (0,0,0): od y=-0.45 do y=0.45
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
    gluCylinder(q, 0.25, 0.25, 0.9, 24, 8);
    if (caps) {
        // Rozszerzenie: gluDisk lezy w plaszczyznie z=0 z normalna +Z (lokalnie).
        // Gorne denko: przesuwamy na z=0.9, normalna +Z = swiatowe +Y (na zewnatrz).
        glPushMatrix();
        glTranslatef(0.0f, 0.0f, 0.9f);
        gluDisk(q, 0.0, 0.25, 24, 1);
        glPopMatrix();
        // Dolne denko musi patrzec w -Y: odwracamy orientacje normalnych kwadryki.
        gluQuadricOrientation(q, GLU_INSIDE);
        gluDisk(q, 0.0, 0.25, 24, 1);
        gluQuadricOrientation(q, GLU_OUTSIDE);
    }
    glPopMatrix();
}

void drawCone(GLUquadric* q) {
    glPushMatrix();
    glTranslatef(0.9f, -0.45f, 0.0f);             // podstawa na y=-0.45, os w x=0.9, z=0
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
    gluCylinder(q, 0.35, 0.0, 0.9, 24, 8);        // topRadius = 0 -> stozek, wierzcholek y=0.45
    glPopMatrix();
}

void drawThreeSolids(GLUquadric* q, const AppState& s) {
    glColor3f(0.05f, 0.83f, 0.79f);               // turkusowa sfera
    drawSphere(q, s.lowResSphere ? 8 : 24);
    glColor3f(1.0f, 0.55f, 0.08f);                // pomaranczowy walec
    drawCylinder(q, s.cylinderCaps);
    glColor3f(0.95f, 0.16f, 0.13f);               // czerwony stozek
    drawCone(q);
}

// ============================================================================
// ZADANIE 10 - oswietlenie: konfiguracja raz, pozycja w kazdej klatce
// ============================================================================
void configureLightingOnce() {
    const GLfloat globalAmbient[4] = {0.12f, 0.12f, 0.12f, 1.0f};
    const GLfloat ambient[4] = {0.10f, 0.10f, 0.10f, 1.0f};
    const GLfloat diffuse[4] = {0.85f, 0.85f, 0.85f, 1.0f};
    const GLfloat specular[4] = {1.0f, 1.0f, 1.0f, 1.0f};      // potrzebne w zad. 12
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmbient);
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, specular);
    // ZADANIE 11 - tlumienie swiatla punktowego z odlegloscia
    glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, 1.0f);
    glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, 0.15f);
    glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0.05f);

    glEnable(GL_LIGHT0);
    glEnable(GL_NORMALIZE);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);  // glColor3f -> ambient + diffuse
}

// Wolane po gluLookAt, przed transformacjami bryl: pozycja w ukladzie swiata.
void placeLight(const AppState& s) {
    if (s.task == 10) {
        const GLfloat direction[4] = {2.0f, 3.0f, 2.0f, 0.0f};  // w = 0: kierunek DO zrodla
        glLightfv(GL_LIGHT0, GL_POSITION, direction);
    } else if (s.task >= 11) {
        const GLfloat position[4] = {s.lightPosition.x, s.lightPosition.y, s.lightPosition.z,
                                     s.pointLight ? 1.0f : 0.0f};  // w = 1: punkt w przestrzeni
        glLightfv(GL_LIGHT0, GL_POSITION, position);
    }
}

// ZADANIE 11 - znacznik zrodla swiatla (bez oswietlenia i bez transformacji bryl)
void drawLightMarker(const AppState& s) {
    if (s.task < 11 || !s.pointLight) return;    // swiatlo kierunkowe nie ma polozenia
    GLfloat previousSize = 1.0f;
    glGetFloatv(GL_POINT_SIZE, &previousSize);
    glDisable(GL_LIGHTING);
    glPointSize(8.0f);
    glColor3f(1.0f, 1.0f, 0.75f);
    glBegin(GL_POINTS);
    glVertex3f(s.lightPosition.x, s.lightPosition.y, s.lightPosition.z);
    glEnd();
    glPointSize(previousSize);
}

// ============================================================================
// ZADANIE 12 - ta sama barwa, rozny polysk (GL_COLOR_MATERIAL wylaczony)
// ============================================================================
void drawMaterialSphere(GLUquadric* q, float x, float shininess, bool specularOn) {
    const GLfloat ambientDiffuse[4] = {0.12f, 0.35f, 0.70f, 1.0f};
    const GLfloat specular[4] = {0.95f, 0.95f, 0.95f, 1.0f};
    const GLfloat noSpecular[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, ambientDiffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specularOn ? specular : noSpecular);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shininess);
    glPushMatrix();
    glTranslatef(x, 0.0f, 0.0f);
    gluSphere(q, 0.5, 32, 20);
    glPopMatrix();
}

void drawMaterialComparison(GLUquadric* q, const AppState& s) {
    glDisable(GL_COLOR_MATERIAL);                 // glColor nie nadpisze materialu
    drawMaterialSphere(q, -0.7f, 8.0f, true);
    drawMaterialSphere(q, 0.7f, s.rightShininess, s.rightSpecular);
    // Powrot do stanu dla obiektow barwionych glColor3f; specular materialu zerujemy,
    // zeby nie zostal "przyklejony" do pozostalych obiektow.
    const GLfloat noSpecular[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, noSpecular);
    glEnable(GL_COLOR_MATERIAL);
}

void drawScene(GLUquadric* q, const AppState& s) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    setCamera(s.camera);
    placeLight(s);                                // po kamerze, przed brylami
    drawAxes();

    if (s.task >= 10 && s.lighting) glEnable(GL_LIGHTING);
    else glDisable(GL_LIGHTING);                  // zad. 9: same kolory, bez swiatla

    if (s.task == 12) drawMaterialComparison(q, s);
    else drawThreeSolids(q, s);

    glDisable(GL_LIGHTING);
    drawLightMarker(s);
}

// ---------------------------------------------------------------------------
// Sterowanie
// ---------------------------------------------------------------------------
void updateHeldKeys(AppState& s, float dt) {
    using K = sf::Keyboard;
    OrbitCamera& c = s.camera;
    if (K::isKeyPressed(K::F)) c.azimuth -= 1.2f * dt;
    if (K::isKeyPressed(K::H)) c.azimuth += 1.2f * dt;
    if (K::isKeyPressed(K::T)) c.elevation += 1.2f * dt;
    if (K::isKeyPressed(K::G)) c.elevation -= 1.2f * dt;
    if (K::isKeyPressed(K::O)) c.radius -= 2.0f * dt;
    if (K::isKeyPressed(K::P)) c.radius += 2.0f * dt;
    c.elevation = clampf(c.elevation, -1.4f, 1.4f);
    c.radius = clampf(c.radius, 2.5f, 10.0f);

    if (s.task >= 11) {                           // ruch swiatla: 1.5 j/s
        sf::Vector3f& p = s.lightPosition;
        const float step = 1.5f * dt;
        if (K::isKeyPressed(K::A)) p.x -= step;
        if (K::isKeyPressed(K::D)) p.x += step;
        if (K::isKeyPressed(K::S)) p.y -= step;
        if (K::isKeyPressed(K::W)) p.y += step;
        if (K::isKeyPressed(K::Q)) p.z -= step;
        if (K::isKeyPressed(K::E)) p.z += step;
        p.x = clampf(p.x, -4.0f, 4.0f);
        p.y = clampf(p.y, 0.5f, 4.0f);
        p.z = clampf(p.z, -4.0f, 4.0f);
    }
}

bool handleKeyPressed(sf::Keyboard::Key key, AppState& s) {
    switch (key) {
    case sf::Keyboard::F1: case sf::Keyboard::F2: case sf::Keyboard::F3: case sf::Keyboard::F4:
        s.task = 9 + static_cast<int>(key - sf::Keyboard::F1);
        s.lighting = true;
        s.smoothNormals = true;
        s.wireframe = false;
        s.lowResSphere = false;
        s.pointLight = true;
        std::cout << "Zadanie " << s.task << '\n';
        break;
    case sf::Keyboard::N: s.camera = OrbitCamera(); break;
    case sf::Keyboard::Num9: s.perspective = true; return true;
    case sf::Keyboard::Num0: s.perspective = false; return true;
    case sf::Keyboard::V: if (s.perspective) { s.fov = clampf(s.fov - 5.0f, 25.0f, 90.0f); return true; } break;
    case sf::Keyboard::B: if (s.perspective) { s.fov = clampf(s.fov + 5.0f, 25.0f, 90.0f); return true; } break;
    case sf::Keyboard::U: s.lowResSphere = !s.lowResSphere; break;
    case sf::Keyboard::L: s.wireframe = !s.wireframe; break;
    case sf::Keyboard::I: s.cylinderCaps = !s.cylinderCaps; break;
    case sf::Keyboard::M: s.lighting = !s.lighting; break;
    case sf::Keyboard::C: s.smoothNormals = !s.smoothNormals; break;
    case sf::Keyboard::Space: if (s.task >= 11) s.lightPosition = LIGHT_START; break;
    case sf::Keyboard::Y: if (s.task >= 11) s.pointLight = !s.pointLight; break;
    case sf::Keyboard::Z: s.rightShininess = clampf(s.rightShininess - 8.0f, 0.0f, 128.0f); break;
    case sf::Keyboard::X: s.rightShininess = clampf(s.rightShininess + 8.0f, 0.0f, 128.0f); break;
    case sf::Keyboard::K: s.rightSpecular = !s.rightSpecular; break;
    default: break;
    }
    return false;
}

std::string makeTitle(const AppState& s) {
    std::string t = "Grafika 3D - Lab 3 - zadanie " + std::to_string(s.task);
    if (s.task == 9) t += std::string(" | ") + (s.wireframe ? "GLU_LINE" : "GLU_FILL") +
                          (s.lowResSphere ? ", sfera 8 wycinkow" : "");
    if (s.task >= 10) t += std::string(" | swiatlo ") + (s.lighting ? "ON" : "OFF") +
                           ", normalne " + (s.smoothNormals ? "GLU_SMOOTH" : "GLU_NONE");
    if (s.task >= 11) t += s.pointLight ? " | punktowe" : " | kierunkowe";
    if (s.task == 12) t += " | polysk prawej: " + std::to_string(static_cast<int>(s.rightShininess)) +
                           (s.rightSpecular ? "" : " (specular 0)");
    return t;
}

} // namespace

int main() {
    const sf::ContextSettings settings(24, 0, 0, 2, 1);
    sf::RenderWindow window(sf::VideoMode(1000, 700), "Grafika 3D - Lab 3", sf::Style::Default, settings);
    window.setVerticalSyncEnabled(true);
    window.setKeyRepeatEnabled(false);
    window.setActive(true);

    glClearColor(0.10f, 0.14f, 0.20f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glShadeModel(GL_SMOOTH);
    configureLightingOnce();

    // Jedna kwadryka na caly program - tworzona po utworzeniu kontekstu OpenGL.
    GLUquadric* quadric = gluNewQuadric();
    if (!quadric) {
        std::cerr << "Nie udalo sie utworzyc kwadryki GLU.\n";
        return 1;
    }
    gluQuadricDrawStyle(quadric, GLU_FILL);
    gluQuadricNormals(quadric, GLU_SMOOTH);

    AppState state;
    configureProjection(window.getSize().x, window.getSize().y, state);
    std::cout << "F1-F4 zadania 9-12 | F/H/T/G/O/P/N kamera, 9/0 V/B projekcja\n"
              << "U/L/I bryly | M swiatlo, C normalne | A/D S/W Q/E swiatlo, Spacja reset, Y typ\n"
              << "Z/X polysk, K specular | Esc\n";

    sf::Clock clock;
    std::string lastTitle;
    while (window.isOpen()) {
        const float dt = std::min(clock.restart().asSeconds(), 0.05f);
        bool projectionChanged = false;
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) window.close();
            if (event.type == sf::Event::Resized) projectionChanged = true;
            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Escape) window.close();
                else projectionChanged |= handleKeyPressed(event.key.code, state);
            }
            if (event.type == sf::Event::MouseWheelScrolled &&
                event.mouseWheelScroll.wheel == sf::Mouse::VerticalWheel) {
                state.camera.radius = clampf(state.camera.radius - 0.35f * event.mouseWheelScroll.delta,
                                             2.5f, 10.0f);
            }
        }
        if (!window.isOpen()) break;

        updateHeldKeys(state, dt);
        if (projectionChanged) configureProjection(window.getSize().x, window.getSize().y, state);

        // Eksperymenty zad. 9 i 10 zmieniaja ustawienia tej samej kwadryki.
        gluQuadricDrawStyle(quadric, state.wireframe ? GLU_LINE : GLU_FILL);
        gluQuadricNormals(quadric, state.smoothNormals ? GLU_SMOOTH : GLU_NONE);
        if (!state.smoothNormals) glNormal3f(0.0f, 1.0f, 0.0f);   // stala normalna przy GLU_NONE

        const std::string title = makeTitle(state);
        if (title != lastTitle) { window.setTitle(title); lastTitle = title; }

        drawScene(quadric, state);
        window.display();
    }

    gluDeleteQuadric(quadric);                    // zwalniamy, gdy kontekst jeszcze istnieje
    return 0;
}
