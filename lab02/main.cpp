// ============================================================================
// Grafika 3D - Laboratorium 2: transformacje obiektow i sterowanie kamera
// Zadania 5-8 w jednym programie. C++11+, SFML 2.5.1, OpenGL 2.1 + GLU.
//
// Sterowanie:
//   F1/F2/F3/F4   stan po zadaniu 5/6/7/8 (start: zadanie 8)
//   Strzalki      przesuwanie obiektu 1 w X i Y (0.8 j/s)
//   J / L         obrot obiektu 1 wokol Y (60 st/s)
//   I / K         skala obiektu 1 +/- (0.6/s, zakres 0.3-2.0)
//   R             reset obiektu 1 (nie kamery)
//   X             eksperyment: kolejnosc R*T zamiast T*R dla obiektu 1
//   Z             eksperyment: kolejnosc R*T zamiast T*R dla obiektu 2 (zad. 6+)
//   Y             automatyczny obrot obiektu 2 (rozszerzenie zad. 6)
//   F/H T/G O/P   kamera: azymut, elewacja, promien (zad. 7+)
//   N             reset kamery (zad. 7+)
//   kolko myszy   przyblizanie (rozszerzenie zad. 7)
//   9 / 0         projekcja perspektywiczna / ortograficzna (zad. 8)
//   V / B         fov -/+ 5 stopni w perspektywie (zad. 8)
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

// ZADANIE 5 - parametry sterowanego obiektu (trojkat + szkielet szescianu)
struct ObjectState {
    sf::Vector3f position{0.0f, 0.0f, 0.0f};   // od zad. 6: dodatkowe przesuniecie
    float rotationY = 0.0f;                    // stopnie
    float objectScale = 1.0f;
};

// ZADANIE 7 - kamera orbitalna (katy w radianach)
struct OrbitCamera {
    float radius = 3.9f;
    float azimuth = 0.59f;
    float elevation = 0.39f;
};

struct AppState {
    int task = 8;
    ObjectState object;
    OrbitCamera camera;
    bool perspective = true;                   // zad. 8
    float fov = 45.0f;
    bool swapFirst = false;                    // eksperyment z zad. 5
    bool swapSecond = false;                   // eksperyment z zad. 6
    bool spinSecond = true;                    // rozszerzenie zad. 6
    float secondSpin = 0.0f;                   // stopnie
};

// ---------------------------------------------------------------------------
// Baza z laboratorium 1
// ---------------------------------------------------------------------------
void drawAxes() {
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    glColor3f(1.0f, 0.2f, 0.2f); glVertex3f(0, 0, 0); glVertex3f(1.5f, 0, 0);
    glColor3f(0.2f, 1.0f, 0.2f); glVertex3f(0, 0, 0); glVertex3f(0, 1.5f, 0);
    glColor3f(0.2f, 0.5f, 1.0f); glVertex3f(0, 0, 0); glVertex3f(0, 0, 1.5f);
    glEnd();
    glLineWidth(1.0f);
}

void drawTriangle() {
    glBegin(GL_TRIANGLES);
    glColor3f(1.0f, 0.0f, 0.0f); glVertex3f(-0.4f, -0.3f, 0.0f);
    glColor3f(0.0f, 1.0f, 0.0f); glVertex3f( 0.4f, -0.3f, 0.0f);
    glColor3f(0.0f, 0.0f, 1.0f); glVertex3f( 0.0f,  0.4f, 0.0f);
    glEnd();
}

// ZADANIE 6 - kolor przekazywany argumentami, geometria zawsze ta sama (bok 1.2)
void drawWireCube(float r, float g, float b) {
    const GLfloat v[8][3] = {
        {-0.6f, -0.6f, -0.6f}, { 0.6f, -0.6f, -0.6f}, { 0.6f,  0.6f, -0.6f}, {-0.6f,  0.6f, -0.6f},
        {-0.6f, -0.6f,  0.6f}, { 0.6f, -0.6f,  0.6f}, { 0.6f,  0.6f,  0.6f}, {-0.6f,  0.6f,  0.6f}};
    const int edges[12][2] = {
        {0, 1}, {1, 2}, {2, 3}, {3, 0}, {4, 5}, {5, 6}, {6, 7}, {7, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};
    glLineWidth(2.0f);
    glColor3f(r, g, b);
    glBegin(GL_LINES);
    for (int i = 0; i < 12; ++i) {
        glVertex3fv(v[edges[i][0]]);
        glVertex3fv(v[edges[i][1]]);
    }
    glEnd();
    glLineWidth(1.0f);
}

// ============================================================================
// ZADANIA 5-6 - transformacje i stos macierzy
// ============================================================================
void drawFirstObject(const AppState& s) {
    const ObjectState& o = s.object;
    const float baseX = (s.task >= 6) ? -0.9f : 0.0f;   // od zad. 6 srodek w (-0.9, 0, 0)

    glPushMatrix();                                     // zachowujemy sama macierz kamery
    if (!s.swapFirst) {
        // T * R * S: na wierzcholek dziala najpierw skala, potem obrot wokol wlasnego
        // srodka, na koncu przesuniecie.
        glTranslatef(baseX + o.position.x, o.position.y, o.position.z);
        glRotatef(o.rotationY, 0.0f, 1.0f, 0.0f);
    } else {
        // R * T * S: przesuniecie tez zostaje obrocone - obiekt krazy po luku wokol (0,0,0).
        glRotatef(o.rotationY, 0.0f, 1.0f, 0.0f);
        glTranslatef(baseX + o.position.x, o.position.y, o.position.z);
    }
    glScalef(o.objectScale, o.objectScale, o.objectScale);
    drawTriangle();
    drawWireCube(0.90f, 0.90f, 0.90f);                  // jasnoszary
    glPopMatrix();                                      // nic nie przecieka do obiektu 2
}

void drawSecondObject(const AppState& s) {
    const float angle = 35.0f + s.secondSpin;
    glPushMatrix();
    if (!s.swapSecond) {
        glTranslatef(0.9f, 0.0f, 0.0f);
        glRotatef(angle, 0.0f, 1.0f, 0.0f);
    } else {
        glRotatef(angle, 0.0f, 1.0f, 0.0f);             // srodek ucieka z (0.9, 0, 0)
        glTranslatef(0.9f, 0.0f, 0.0f);
    }
    glScalef(0.65f, 0.65f, 0.65f);
    drawWireCube(0.05f, 0.85f, 0.78f);                  // turkusowy
    glPopMatrix();
}

// ============================================================================
// ZADANIE 7 - kamera orbitalna ze wspolrzednych sferycznych
// ============================================================================
void setCamera(const AppState& s) {
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    if (s.task < 7) {                                   // zad. 5-6: kamera z laboratorium 1
        gluLookAt(2.0, 1.5, 3.0,  0.0, 0.0, 0.0,  0.0, 1.0, 0.0);
        return;
    }
    const OrbitCamera& c = s.camera;
    const float x = c.radius * std::cos(c.elevation) * std::sin(c.azimuth);
    const float y = c.radius * std::sin(c.elevation);
    const float z = c.radius * std::cos(c.elevation) * std::cos(c.azimuth);
    gluLookAt(x, y, z,  0.0, 0.0, 0.0,  0.0, 1.0, 0.0);
}

// ============================================================================
// ZADANIE 8 - projekcja perspektywiczna albo ortograficzna
// ============================================================================
void configureProjection(unsigned width, unsigned height, const AppState& s) {
    const unsigned w = std::max(1u, width), h = std::max(1u, height);
    const double aspect = static_cast<double>(w) / static_cast<double>(h);
    glViewport(0, 0, static_cast<GLsizei>(w), static_cast<GLsizei>(h));

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    if (s.task >= 8 && !s.perspective) {
        glOrtho(-1.75 * aspect, 1.75 * aspect, -1.75, 1.75, 0.1, 100.0);
    } else {
        gluPerspective(s.task >= 8 ? s.fov : 45.0f, aspect, 0.1, 100.0);
    }
    glMatrixMode(GL_MODELVIEW);                          // nie nadpisujemy projekcji przy rysowaniu
}

void drawScene(const AppState& s) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    setCamera(s);                                        // raz na klatke, przed obiektami
    drawAxes();                                          // poza parami push/pop obiektow
    drawFirstObject(s);
    if (s.task >= 6) drawSecondObject(s);
}

// ---------------------------------------------------------------------------
// Sterowanie: przytrzymane klawisze (deltaTime) i pojedyncze nacisniecia
// ---------------------------------------------------------------------------
void updateHeldKeys(AppState& s, float dt) {
    using K = sf::Keyboard;
    ObjectState& o = s.object;
    if (K::isKeyPressed(K::Left))  o.position.x -= 0.8f * dt;
    if (K::isKeyPressed(K::Right)) o.position.x += 0.8f * dt;
    if (K::isKeyPressed(K::Up))    o.position.y += 0.8f * dt;
    if (K::isKeyPressed(K::Down))  o.position.y -= 0.8f * dt;
    if (K::isKeyPressed(K::J)) o.rotationY -= 60.0f * dt;
    if (K::isKeyPressed(K::L)) o.rotationY += 60.0f * dt;
    if (K::isKeyPressed(K::I)) o.objectScale += 0.6f * dt;
    if (K::isKeyPressed(K::K)) o.objectScale -= 0.6f * dt;
    o.objectScale = clampf(o.objectScale, 0.3f, 2.0f);

    if (s.task >= 6 && s.spinSecond) s.secondSpin = std::fmod(s.secondSpin + 45.0f * dt, 360.0f);

    if (s.task >= 7) {
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
}

// Zwraca true, gdy trzeba przeliczyc macierz projekcji.
bool handleKeyPressed(sf::Keyboard::Key key, AppState& s) {
    switch (key) {
    case sf::Keyboard::F1: case sf::Keyboard::F2: case sf::Keyboard::F3: case sf::Keyboard::F4:
        s.task = 5 + static_cast<int>(key - sf::Keyboard::F1);
        s.swapFirst = s.swapSecond = false;
        std::cout << "Zadanie " << s.task << '\n';
        return true;
    case sf::Keyboard::R: s.object = ObjectState(); break;          // tylko obiekt
    case sf::Keyboard::N: if (s.task >= 7) s.camera = OrbitCamera(); break;  // tylko kamera
    case sf::Keyboard::X:
        s.swapFirst = !s.swapFirst;
        std::cout << "Obiekt 1: " << (s.swapFirst ? "R*T*S (luk wokol poczatku)" : "T*R*S") << '\n';
        break;
    case sf::Keyboard::Z:
        if (s.task >= 6) {
            s.swapSecond = !s.swapSecond;
            std::cout << "Obiekt 2: " << (s.swapSecond ? "R*T*S" : "T*R*S") << '\n';
        }
        break;
    case sf::Keyboard::Y: s.spinSecond = !s.spinSecond; break;
    case sf::Keyboard::Num9: if (s.task >= 8) { s.perspective = true; return true; } break;
    case sf::Keyboard::Num0: if (s.task >= 8) { s.perspective = false; return true; } break;
    case sf::Keyboard::V:
        if (s.task >= 8 && s.perspective) { s.fov = clampf(s.fov - 5.0f, 25.0f, 90.0f); return true; }
        break;
    case sf::Keyboard::B:
        if (s.task >= 8 && s.perspective) { s.fov = clampf(s.fov + 5.0f, 25.0f, 90.0f); return true; }
        break;
    default: break;
    }
    return false;
}

std::string makeTitle(const AppState& s) {
    std::string t = "Grafika 3D - Lab 2 - zadanie " + std::to_string(s.task);
    if (s.task >= 8) {
        t += s.perspective ? " | perspektywa, fov " + std::to_string(static_cast<int>(s.fov))
                           : std::string(" | rzut ortograficzny");
    }
    if (s.swapFirst || s.swapSecond) t += " | eksperyment R*T";
    return t;
}

} // namespace

int main() {
    const sf::ContextSettings settings(24, 0, 0, 2, 1);
    sf::RenderWindow window(sf::VideoMode(1000, 700), "Grafika 3D - Lab 2", sf::Style::Default, settings);
    window.setVerticalSyncEnabled(true);
    window.setKeyRepeatEnabled(false);      // jedno KeyPressed na nacisniecie
    window.setActive(true);

    glClearColor(0.10f, 0.14f, 0.20f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glShadeModel(GL_SMOOTH);

    AppState state;
    configureProjection(window.getSize().x, window.getSize().y, state);
    std::cout << "F1-F4 zadania 5-8 | strzalki/J/L/I/K/R obiekt | F/H/T/G/O/P/N kamera\n"
              << "9/0 projekcja, V/B fov | X/Z kolejnosc T i R | Y obrot obiektu 2 | Esc\n";

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
            // Rozszerzenie zad. 7: kolko myszy, te same granice co O/P.
            if (event.type == sf::Event::MouseWheelScrolled && state.task >= 7 &&
                event.mouseWheelScroll.wheel == sf::Mouse::VerticalWheel) {
                state.camera.radius = clampf(state.camera.radius - 0.35f * event.mouseWheelScroll.delta,
                                             2.5f, 10.0f);
            }
        }
        if (!window.isOpen()) break;

        updateHeldKeys(state, dt);
        if (projectionChanged) configureProjection(window.getSize().x, window.getSize().y, state);

        const std::string title = makeTitle(state);
        if (title != lastTitle) { window.setTitle(title); lastTitle = title; }

        drawScene(state);
        window.display();
    }
    return 0;
}
