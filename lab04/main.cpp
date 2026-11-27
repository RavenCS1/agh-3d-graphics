// ============================================================================
// Grafika 3D - Laboratorium 4: siatki trojkatow, normalne, animacja, przezroczystosc
// Zadania 13-16 w jednym programie. C++11+, SFML 2.5.1, OpenGL 2.1 + GLU.
//
// Sterowanie:
//   F1/F2/F3/F4   stan po zadaniu 13/14/15/16 (start: zadanie 16)
//   F/H T/G O/P   kamera: azymut, elewacja, promien;  N reset kamery
//   9 / 0, V / B  perspektywa / orto, fov
//   L             wypelnienie / szkielet siatki (zad. 13)
//   C             normalne flat / smooth (zad. 14+)
//   K             pokaz normalne (zad. 14+)
//   M             oswietlenie wl./wyl. (zad. 14+)
//   Spacja        pauza animacji (zad. 15+)
//   Z / X         amplituda -/+ 0.05, zakres 0-0.6 (zad. 15+)
//   R             reset fali: amplituda 0.30, faza 0, wznowienie (zad. 15+)
//   LPM + mysz    obrot kamery, kolko - promien (zad. 15+)
//   Y             przezroczysta warstwa wl./wyl. (zad. 16)
//   A / D         alfa warstwy -/+ 0.10, zakres 0.10-0.90 (zad. 16)
//   U             eksperyment: warstwa rysowana bez testu glebokosci (zad. 16)
//   F12           zapis klatki do Lab4_capture.png (rozszerzenie zad. 16)
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
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

const float PI = 3.14159265358979323846f;
const int GRID = 21;                 // 21 x 21 wierzcholkow -> 20 x 20 komorek -> 800 trojkatow
const float BASE_Y = -0.35f;

float clampf(float v, float lo, float hi) { return std::max(lo, std::min(v, hi)); }

// ============================================================================
// ZADANIE 14 - wektory: iloczyn wektorowy i normalizacja
// ============================================================================
struct Vec3 {
    float x, y, z;
    Vec3() : x(0), y(0), z(0) {}
    Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
};
Vec3 operator+(const Vec3& a, const Vec3& b) { return Vec3(a.x + b.x, a.y + b.y, a.z + b.z); }
Vec3 operator-(const Vec3& a, const Vec3& b) { return Vec3(a.x - b.x, a.y - b.y, a.z - b.z); }
Vec3 operator*(const Vec3& a, float s) { return Vec3(a.x * s, a.y * s, a.z * s); }

Vec3 cross(const Vec3& u, const Vec3& v) {
    return Vec3(u.y * v.z - u.z * v.y, u.z * v.x - u.x * v.z, u.x * v.y - u.y * v.x);
}

Vec3 normalize(const Vec3& v) {
    const float len = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    if (len < 1.0e-6f) return Vec3(0.0f, 1.0f, 0.0f);   // zabezpieczenie dlugosci ~0
    return v * (1.0f / len);
}

// ============================================================================
// ZADANIE 13 - dane siatki oddzielone od rysowania
// ============================================================================
struct Mesh {
    std::vector<Vec3> positions;         // 441 pozycji
    std::vector<Vec3> vertexNormals;     // ta sama liczba elementow
    Mesh() : positions(GRID * GRID), vertexNormals(GRID * GRID) {}
};

int index(int row, int col) { return row * GRID + col; }   // wiersz wzdluz Z, kolumna wzdluz X

// Komorka (row, col): narozniki a=(row,col), b=(row,col+1), c=(row+1,col+1), d=(row+1,col).
// Trojkaty (a, d, c) i (a, c, b): cross(d-a, c-a) oraz cross(c-a, b-a) wskazuja +Y,
// czyli patrzac od strony +Y wierzcholki ida przeciwnie do wskazowek zegara (CCW).
void cellTriangles(int row, int col, int tri[2][3]) {
    const int a = index(row, col), b = index(row, col + 1);
    const int c = index(row + 1, col + 1), d = index(row + 1, col);
    tri[0][0] = a; tri[0][1] = d; tri[0][2] = c;
    tri[1][0] = a; tri[1][1] = c; tri[1][2] = b;
}

Vec3 triangleNormal(const Mesh& m, int a, int b, int c) {
    return normalize(cross(m.positions[b] - m.positions[a], m.positions[c] - m.positions[a]));
}

struct Wave {
    float amplitude = 0.30f;
    float phase = 0.0f;
    bool paused = false;
};

// ZADANIE 13: y = -0.35; ZADANIE 14: fala statyczna; ZADANIE 15: fala z faza.
float heightAt(float x, float z, int task, const Wave& w) {
    if (task == 13) return BASE_Y;
    if (task == 14) return BASE_Y + 0.30f * std::sin(PI * x) * std::cos(PI * z);
    return BASE_Y + w.amplitude * std::sin(PI * x - w.phase) * std::cos(PI * z + w.phase);
}

void buildMesh(Mesh& m, int task, const Wave& w) {
    for (int row = 0; row < GRID; ++row) {
        const float z = -1.0f + 0.1f * static_cast<float>(row);                 // odstep 0.1 jednostki
        for (int col = 0; col < GRID; ++col) {
            const float x = -1.0f + 0.1f * static_cast<float>(col);
            m.positions[index(row, col)] = Vec3(x, heightAt(x, z, task, w), z);
        }
    }
    // Normalne wierzcholkowe: suma normalnych sasiednich trojkatow, potem normalizacja.
    std::fill(m.vertexNormals.begin(), m.vertexNormals.end(), Vec3());
    for (int row = 0; row < GRID - 1; ++row) {
        for (int col = 0; col < GRID - 1; ++col) {
            int tri[2][3];
            cellTriangles(row, col, tri);
            for (int t = 0; t < 2; ++t) {
                const Vec3 n = triangleNormal(m, tri[t][0], tri[t][1], tri[t][2]);
                for (int k = 0; k < 3; ++k) m.vertexNormals[tri[t][k]] = m.vertexNormals[tri[t][k]] + n;
            }
        }
    }
    for (Vec3& n : m.vertexNormals) n = normalize(n);
}

// ---------------------------------------------------------------------------
// Stan programu, kamera, projekcja
// ---------------------------------------------------------------------------
struct OrbitCamera {
    float radius = 4.5f, azimuth = 0.62f, elevation = 0.58f;
};

struct AppState {
    int task = 16;
    OrbitCamera camera;
    bool perspective = true;
    float fov = 45.0f;
    bool wireframe = false;          // zad. 13
    bool smoothNormals = true;       // zad. 14
    bool showNormals = false;
    bool lighting = true;
    Wave wave;                       // zad. 15
    bool dragging = false;
    sf::Vector2i lastMouse;
    bool showOverlay = true;         // zad. 16
    float overlayAlpha = 0.45f;
    bool overlayDepthTest = true;
    bool screenshot = false;
};

void setCamera(const OrbitCamera& c) {
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(c.radius * std::cos(c.elevation) * std::sin(c.azimuth),
              c.radius * std::sin(c.elevation),
              c.radius * std::cos(c.elevation) * std::cos(c.azimuth),
              0.0, -0.2, 0.0,  0.0, 1.0, 0.0);
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

void configureLighting() {
    const GLfloat globalAmbient[4] = {0.14f, 0.14f, 0.14f, 1.0f};
    const GLfloat ambient[4] = {0.08f, 0.08f, 0.08f, 1.0f};
    const GLfloat diffuse[4] = {0.90f, 0.90f, 0.90f, 1.0f};
    const GLfloat specular[4] = {0.70f, 0.70f, 0.70f, 1.0f};
    const GLfloat materialSpecular[4] = {0.25f, 0.25f, 0.25f, 1.0f};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmbient);
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, specular);
    glEnable(GL_LIGHT0);
    glEnable(GL_NORMALIZE);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, materialSpecular);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 30.0f);
}

void drawAxes() {
    glDisable(GL_LIGHTING);
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    glColor3f(1.0f, 0.2f, 0.2f); glVertex3f(0, 0, 0); glVertex3f(1.4f, 0, 0);
    glColor3f(0.2f, 1.0f, 0.2f); glVertex3f(0, 0, 0); glVertex3f(0, 1.4f, 0);
    glColor3f(0.2f, 0.5f, 1.0f); glVertex3f(0, 0, 0); glVertex3f(0, 0, 1.4f);
    glEnd();
    glLineWidth(1.0f);
}

// ============================================================================
// ZADANIA 13-14 - rysowanie siatki (GL_TRIANGLES), normalne flat / smooth
// ============================================================================
void drawSurface(const Mesh& m, const AppState& s) {
    glPolygonMode(GL_FRONT_AND_BACK, s.wireframe ? GL_LINE : GL_FILL);
    glBegin(GL_TRIANGLES);
    for (int row = 0; row < GRID - 1; ++row) {
        for (int col = 0; col < GRID - 1; ++col) {
            if (s.task == 13) {                         // szachownica dwoch odcieni turkusu
                if ((row + col) % 2 == 0) glColor3f(0.10f, 0.62f, 0.70f);
                else glColor3f(0.20f, 0.84f, 0.88f);
            } else {
                glColor3f(0.12f, 0.75f, 0.81f);         // jednolity turkusowy material
            }
            int tri[2][3];
            cellTriangles(row, col, tri);
            for (int t = 0; t < 2; ++t) {
                const Vec3 faceNormal = triangleNormal(m, tri[t][0], tri[t][1], tri[t][2]);
                for (int k = 0; k < 3; ++k) {
                    const int i = tri[t][k];
                    const Vec3& n = s.smoothNormals ? m.vertexNormals[i] : faceNormal;
                    glNormal3f(n.x, n.y, n.z);
                    glVertex3f(m.positions[i].x, m.positions[i].y, m.positions[i].z);
                }
            }
        }
    }
    glEnd();
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);          // nie psujemy innych obiektow
}

// ZADANIE 14 - wizualizacja normalnych dla co czwartego wierzcholka (odcinek 0.15)
void drawNormalLines(const Mesh& m) {
    glDisable(GL_LIGHTING);
    glLineWidth(1.5f);
    glColor3f(1.0f, 0.40f, 0.20f);
    glBegin(GL_LINES);
    for (std::size_t i = 0; i < m.positions.size(); i += 4) {
        const Vec3 tip = m.positions[i] + m.vertexNormals[i] * 0.15f;
        glVertex3f(m.positions[i].x, m.positions[i].y, m.positions[i].z);
        glVertex3f(tip.x, tip.y, tip.z);
    }
    glEnd();
    glLineWidth(1.0f);
}

// ============================================================================
// ZADANIE 16 - przezroczysta warstwa rysowana PO nieprzezroczystej powierzchni
// ============================================================================
void drawTransparentLayer(const AppState& s) {
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);                              // nie zapisujemy glebokosci...
    if (!s.overlayDepthTest) glDisable(GL_DEPTH_TEST);  // ...ale test zostaje (chyba ze eksperyment)

    glColor4f(0.45f, 0.80f, 1.0f, s.overlayAlpha);      // jasnoniebieski
    glBegin(GL_QUADS);                                  // 1.7 x 1.7, srodek (0, -0.35, 0)
    glVertex3f(-0.85f, BASE_Y, -0.85f);
    glVertex3f(-0.85f, BASE_Y,  0.85f);
    glVertex3f( 0.85f, BASE_Y,  0.85f);
    glVertex3f( 0.85f, BASE_Y, -0.85f);
    glEnd();

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);                               // przywrocenie zapisu glebokosci
    glDisable(GL_BLEND);
}

void drawScene(const Mesh& m, const AppState& s) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    setCamera(s.camera);
    const GLfloat lightPosition[4] = {1.5f, 2.2f, 1.3f, 1.0f};   // punktowe, po kamerze
    glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);

    drawAxes();
    if (s.task >= 14 && s.lighting) glEnable(GL_LIGHTING);
    else glDisable(GL_LIGHTING);                        // zad. 13 bez oswietlenia
    drawSurface(m, s);
    glDisable(GL_LIGHTING);

    if (s.task >= 14 && s.showNormals) drawNormalLines(m);
    if (s.task >= 16 && s.showOverlay) drawTransparentLayer(s);
}

// ============================================================================
// ZADANIE 16 - rozszerzenie: zapis klatki z tylnego bufora
// ============================================================================
void saveScreenshot(unsigned width, unsigned height) {
    std::vector<sf::Uint8> pixels(static_cast<std::size_t>(width) * height * 4u);
    glReadBuffer(GL_BACK);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height),
                 GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    sf::Image image;
    image.create(width, height, pixels.data());
    image.flipVertically();                             // w OpenGL pierwszy jest dolny wiersz
    std::cout << (image.saveToFile("Lab4_capture.png") ? "Zapisano Lab4_capture.png\n"
                                                         : "Blad zapisu Lab4_capture.png\n");
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
}

// ZADANIE 15 - mysz: przeciaganie LPM (0.006 rad/piksel) i kolko (0.35 na krok)
void handleMouse(const sf::Event& e, AppState& s) {
    if (s.task < 15) return;
    if (e.type == sf::Event::MouseButtonPressed && e.mouseButton.button == sf::Mouse::Left) {
        s.dragging = true;
        s.lastMouse = sf::Vector2i(e.mouseButton.x, e.mouseButton.y);
    } else if (e.type == sf::Event::MouseButtonReleased && e.mouseButton.button == sf::Mouse::Left) {
        s.dragging = false;
    } else if (e.type == sf::Event::MouseMoved && s.dragging) {
        const int dx = e.mouseMove.x - s.lastMouse.x;   // przesuniecie od poprzedniego zdarzenia
        const int dy = e.mouseMove.y - s.lastMouse.y;
        s.camera.azimuth += 0.006f * static_cast<float>(dx);
        s.camera.elevation = clampf(s.camera.elevation - 0.006f * static_cast<float>(dy), -1.4f, 1.4f);
        s.lastMouse = sf::Vector2i(e.mouseMove.x, e.mouseMove.y);
    } else if (e.type == sf::Event::MouseWheelScrolled && e.mouseWheelScroll.wheel == sf::Mouse::VerticalWheel) {
        s.camera.radius = clampf(s.camera.radius - 0.35f * e.mouseWheelScroll.delta, 2.5f, 10.0f);
    }
}

bool handleKeyPressed(sf::Keyboard::Key key, AppState& s) {
    switch (key) {
    case sf::Keyboard::F1: case sf::Keyboard::F2: case sf::Keyboard::F3: case sf::Keyboard::F4:
        s.task = 13 + static_cast<int>(key - sf::Keyboard::F1);
        std::cout << "Zadanie " << s.task << '\n';
        break;
    case sf::Keyboard::N: s.camera = OrbitCamera(); break;
    case sf::Keyboard::Num9: s.perspective = true; return true;
    case sf::Keyboard::Num0: s.perspective = false; return true;
    case sf::Keyboard::V: if (s.perspective) { s.fov = clampf(s.fov - 5.0f, 25.0f, 90.0f); return true; } break;
    case sf::Keyboard::B: if (s.perspective) { s.fov = clampf(s.fov + 5.0f, 25.0f, 90.0f); return true; } break;
    case sf::Keyboard::L: s.wireframe = !s.wireframe; break;
    case sf::Keyboard::C: if (s.task >= 14) s.smoothNormals = !s.smoothNormals; break;
    case sf::Keyboard::K: if (s.task >= 14) s.showNormals = !s.showNormals; break;
    case sf::Keyboard::M: if (s.task >= 14) s.lighting = !s.lighting; break;
    case sf::Keyboard::Space: if (s.task >= 15) s.wave.paused = !s.wave.paused; break;
    case sf::Keyboard::Z: if (s.task >= 15) s.wave.amplitude = clampf(s.wave.amplitude - 0.05f, 0.0f, 0.60f); break;
    case sf::Keyboard::X: if (s.task >= 15) s.wave.amplitude = clampf(s.wave.amplitude + 0.05f, 0.0f, 0.60f); break;
    case sf::Keyboard::R: if (s.task >= 15) s.wave = Wave(); break;   // 0.30, faza 0, wznowienie
    case sf::Keyboard::Y: if (s.task >= 16) s.showOverlay = !s.showOverlay; break;
    case sf::Keyboard::A: if (s.task >= 16) s.overlayAlpha = clampf(s.overlayAlpha - 0.10f, 0.10f, 0.90f); break;
    case sf::Keyboard::D: if (s.task >= 16) s.overlayAlpha = clampf(s.overlayAlpha + 0.10f, 0.10f, 0.90f); break;
    case sf::Keyboard::U: if (s.task >= 16) s.overlayDepthTest = !s.overlayDepthTest; break;
    case sf::Keyboard::F12: s.screenshot = true; break;
    default: break;
    }
    return false;
}

std::string makeTitle(const AppState& s) {
    std::ostringstream t;
    t << std::fixed << std::setprecision(2) << "Grafika 3D - Lab 4 - zadanie " << s.task;
    if (s.task == 13) t << " | " << (s.wireframe ? "szkielet" : "wypelnienie");
    if (s.task >= 14) t << " | normalne " << (s.smoothNormals ? "smooth" : "flat")
                        << ", swiatlo " << (s.lighting ? "ON" : "OFF");
    if (s.task >= 15) t << " | amplituda " << s.wave.amplitude << (s.wave.paused ? " (pauza)" : "");
    if (s.task >= 16) t << " | warstwa " << (s.showOverlay ? "ON" : "OFF") << ", alfa " << s.overlayAlpha
                        << (s.overlayDepthTest ? "" : ", BEZ testu glebokosci");
    return t.str();
}

} // namespace

int main() {
    const sf::ContextSettings settings(24, 0, 0, 2, 1);
    sf::RenderWindow window(sf::VideoMode(1000, 700), "Grafika 3D - Lab 4", sf::Style::Default, settings);
    window.setVerticalSyncEnabled(true);
    window.setKeyRepeatEnabled(false);
    window.setActive(true);

    glClearColor(0.10f, 0.14f, 0.20f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glShadeModel(GL_SMOOTH);
    configureLighting();

    AppState state;
    Mesh mesh;
    configureProjection(window.getSize().x, window.getSize().y, state);
    std::cout << "F1-F4 zadania 13-16 | kamera F/H/T/G/O/P/N + mysz | 9/0 V/B projekcja\n"
              << "L szkielet | C flat/smooth, K normalne, M swiatlo | Spacja pauza, Z/X amplituda, R reset\n"
              << "Y warstwa, A/D alfa, U bez testu glebokosci, F12 zrzut | Esc\n";

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
            handleMouse(event, state);
        }
        if (!window.isOpen()) break;

        // Kolejnosc: parametry animacji -> dane siatki (pozycje + normalne) -> rysowanie.
        updateHeldKeys(state, dt);
        if (state.task >= 15 && !state.wave.paused) state.wave.phase += 1.5f * dt;
        if (projectionChanged) configureProjection(window.getSize().x, window.getSize().y, state);
        buildMesh(mesh, state.task, state.wave);

        const std::string title = makeTitle(state);
        if (title != lastTitle) { window.setTitle(title); lastTitle = title; }

        drawScene(mesh, state);
        if (state.screenshot) {                          // przed display(): tylny bufor
            saveScreenshot(window.getSize().x, window.getSize().y);
            state.screenshot = false;
        }
        window.display();
    }
    return 0;
}
