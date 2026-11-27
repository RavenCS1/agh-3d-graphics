// ============================================================================
// Grafika 3D - Laboratorium 1: podstawy srodowiska i pierwsze elementy sceny 3D
// Zadania 1-4 w jednym programie (kazde rozwija poprzednie).
// C++11+, SFML 2.5.1, OpenGL 2.1 (fixed-function) + GLU.
//
// Visual Studio (x64, Console App): NuGet sfml_graphics 2.5.1,
//   Linker -> Input: opengl32.lib; glu32.lib (albo arkusz vs/Grafika3D.props).
//
// Sterowanie:
//   1 / 2 / 3 / 4   pokaz stan po zadaniu 1 / 2 / 3 / 4 (start: zadanie 4)
//   C               nastepne polozenie kamery (eksperymenty z zad. 2-4)
//   D               wlacz / wylacz test glebokosci (zad. 4, rozszerzenie)
//   N               ujemne polosie linia przerywana (zad. 2, rozszerzenie)
//   Esc             zamkniecie okna (tak jak przycisk X)
// ============================================================================

#if defined(_WIN32) && !defined(NOMINMAX)
#define NOMINMAX   // windows.h (dolaczany przez SFML/OpenGL.hpp w MSVC) bez makr min/max
#endif
#include <SFML/Graphics.hpp>
#include <SFML/OpenGL.hpp>
#include <GL/glu.h>

#include <iostream>
#include <string>

namespace {

struct AppState {
    int task = 4;
    int camera = 0;              // indeks w tablicy CAMERAS
    bool depthTest = true;       // zad. 4
    bool negativeAxes = true;    // rozszerzenie zad. 2
};

struct CameraPreset {
    double x, y, z;
    const char* description;
};

// Polozenia kamery z eksperymentow. Punkt obserwowany zawsze (0,0,0), pion (0,1,0).
const CameraPreset CAMERAS[] = {
    { 2.0, 1.5,  3.0, "domyslna (2, 1.5, 3)"},
    { 0.0, 0.0,  3.0, "na osi Z (0, 0, 3)"},
    {-2.0, 1.5,  3.0, "po stronie -X (-2, 1.5, 3)"},
    { 2.0, 1.5, -3.0, "od strony -Z (2, 1.5, -3)"},
};
const int CAMERA_COUNT = 4;

// ============================================================================
// ZADANIE 1 - inicjalizacja OpenGL: kolor tla
// ============================================================================
void initOpenGL() {
    glClearColor(0.10f, 0.14f, 0.20f, 1.0f);   // ciemnogranatowe tlo
    glClearDepth(1.0);
    glDepthFunc(GL_LESS);
}

// ============================================================================
// ZADANIE 2 - obszar rysowania i projekcja perspektywiczna
// ============================================================================
void resizeProjection(unsigned width, unsigned height) {
    if (height == 0) height = 1;               // ochrona przed dzieleniem przez zero
    if (width == 0) width = 1;
    glViewport(0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height));

    glMatrixMode(GL_PROJECTION);               // macierz projekcji: jak scena trafia na ekran
    glLoadIdentity();
    const double aspect = static_cast<double>(width) / static_cast<double>(height);
    gluPerspective(45.0, aspect, 0.1, 100.0);

    glMatrixMode(GL_MODELVIEW);                // dalej pracujemy na macierzy widoku/modelu
}

void setCamera(int index) {
    const CameraPreset& c = CAMERAS[index];
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();                          // nowa klatka - bez skladania z poprzednia
    gluLookAt(c.x, c.y, c.z,   0.0, 0.0, 0.0,   0.0, 1.0, 0.0);
}

void drawAxes(bool negativeDashed) {
    glBegin(GL_LINES);
    glColor3f(1.0f, 0.0f, 0.0f); glVertex3f(0, 0, 0); glVertex3f(1.5f, 0, 0);   // X - czerwona
    glColor3f(0.0f, 1.0f, 0.0f); glVertex3f(0, 0, 0); glVertex3f(0, 1.5f, 0);   // Y - zielona
    glColor3f(0.0f, 0.0f, 1.0f); glVertex3f(0, 0, 0); glVertex3f(0, 0, 1.5f);   // Z - niebieska
    glEnd();

    if (!negativeDashed) return;
    // Rozszerzenie: ujemne polosie linia przerywana, potem powrot do linii ciaglej.
    glEnable(GL_LINE_STIPPLE);
    glLineStipple(2, 0x00FF);
    glBegin(GL_LINES);
    glColor3f(1.0f, 0.0f, 0.0f); glVertex3f(0, 0, 0); glVertex3f(-1.5f, 0, 0);
    glColor3f(0.0f, 1.0f, 0.0f); glVertex3f(0, 0, 0); glVertex3f(0, -1.5f, 0);
    glColor3f(0.0f, 0.0f, 1.0f); glVertex3f(0, 0, 0); glVertex3f(0, 0, -1.5f);
    glEnd();
    glDisable(GL_LINE_STIPPLE);
}

// ============================================================================
// ZADANIE 3 - wypelniony trojkat i szkielet szescianu
// ============================================================================
void drawTriangle() {
    glBegin(GL_TRIANGLES);
    glColor3f(1.0f, 0.0f, 0.0f); glVertex3f(-0.4f, -0.3f, 0.0f);
    glColor3f(0.0f, 1.0f, 0.0f); glVertex3f( 0.4f, -0.3f, 0.0f);
    glColor3f(0.0f, 0.0f, 1.0f); glVertex3f( 0.0f,  0.4f, 0.0f);
    glEnd();                                   // kolory sa interpolowane wewnatrz trojkata
}

void drawWireCube() {
    // Bok 1.2, srodek (0,0,0): kazda wspolrzedna wierzcholka to -0.6 albo +0.6.
    const GLfloat v[8][3] = {
        {-0.6f, -0.6f, -0.6f}, { 0.6f, -0.6f, -0.6f}, { 0.6f,  0.6f, -0.6f}, {-0.6f,  0.6f, -0.6f},
        {-0.6f, -0.6f,  0.6f}, { 0.6f, -0.6f,  0.6f}, { 0.6f,  0.6f,  0.6f}, {-0.6f,  0.6f,  0.6f}};
    // 4 krawedzie sciany z = -0.6, 4 sciany z = +0.6 i 4 laczace je krawedzie.
    const int edges[12][2] = {
        {0, 1}, {1, 2}, {2, 3}, {3, 0},
        {4, 5}, {5, 6}, {6, 7}, {7, 4},
        {0, 4}, {1, 5}, {2, 6}, {3, 7}};

    GLfloat previousWidth = 1.0f;
    glGetFloatv(GL_LINE_WIDTH, &previousWidth);
    glLineWidth(2.0f);
    glColor3f(0.90f, 0.90f, 0.90f);
    glBegin(GL_LINES);
    for (int i = 0; i < 12; ++i) {
        glVertex3fv(v[edges[i][0]]);
        glVertex3fv(v[edges[i][1]]);
    }
    glEnd();
    glLineWidth(previousWidth);                // przywrocenie poprzedniej grubosci
}

// ============================================================================
// ZADANIE 4 - bufor glebi: dwa trojkaty o tych samych x, y i roznych z
// ============================================================================
void drawDepthTriangles() {
    // Najpierw trojkat z = +0.25 (blizej kamery domyslnej), dopiero potem z = -0.25.
    glColor3f(1.0f, 0.9f, 0.1f);               // zolty
    glBegin(GL_TRIANGLES);
    glVertex3f(-0.4f, -0.3f, 0.25f); glVertex3f(0.4f, -0.3f, 0.25f); glVertex3f(0.0f, 0.4f, 0.25f);
    glEnd();

    glColor3f(0.65f, 0.25f, 0.95f);            // fioletowy
    glBegin(GL_TRIANGLES);
    glVertex3f(-0.4f, -0.3f, -0.25f); glVertex3f(0.4f, -0.3f, -0.25f); glVertex3f(0.0f, 0.4f, -0.25f);
    glEnd();
    // Bez testu glebokosci fioletowy (rysowany pozniej) zawsze przykrywa zolty.
    // Z testem wygrywa trojkat blizszy kamerze - dla kamery (2,1.5,-3) jest nim fioletowy.
}

void drawScene(const AppState& s) {
    if (s.task >= 4) {
        if (s.depthTest) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);   // oba bufory w kazdej klatce
    } else {
        glDisable(GL_DEPTH_TEST);              // w zad. 1-3 testu glebokosci jeszcze nie ma
        glClear(GL_COLOR_BUFFER_BIT);
    }
    if (s.task == 1) return;                   // zad. 1: samo tlo

    setCamera(s.camera);
    drawAxes(s.negativeAxes);
    if (s.task == 2) return;

    if (s.task == 3) {
        drawTriangle();
    } else {
        drawDepthTriangles();
    }
    drawWireCube();
}

void printState(const AppState& s) {
    std::cout << "Zadanie " << s.task << " | kamera: " << CAMERAS[s.camera].description
              << " | GL_DEPTH_TEST: " << (s.task >= 4 && s.depthTest ? "ON" : "OFF") << '\n';
}

} // namespace

int main() {
    // ZADANIE 1: 24-bitowy bufor glebokosci, brak stencil i antyaliasingu, OpenGL 2.1.
    const sf::ContextSettings settings(24, 0, 0, 2, 1);
    sf::RenderWindow window(sf::VideoMode(800, 600), L"Grafika 3D \u2014 Laboratorium 1",
                            sf::Style::Default, settings);
    window.setVerticalSyncEnabled(true);
    window.setKeyRepeatEnabled(false);
    window.setActive(true);

    const sf::ContextSettings got = window.getSettings();
    std::cout << "OpenGL " << got.majorVersion << '.' << got.minorVersion
              << ", bufor glebokosci: " << got.depthBits << " bitow\n"
              << "1-4 zadanie, C kamera, D test glebokosci, N ujemne osie, Esc wyjscie\n";

    initOpenGL();
    resizeProjection(window.getSize().x, window.getSize().y);

    AppState state;
    printState(state);

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            } else if (event.type == sf::Event::Resized) {
                resizeProjection(event.size.width, event.size.height);
            } else if (event.type == sf::Event::KeyPressed) {
                const sf::Keyboard::Key key = event.key.code;
                if (key == sf::Keyboard::Escape) {
                    window.close();
                } else if (key >= sf::Keyboard::Num1 && key <= sf::Keyboard::Num4) {
                    state.task = 1 + static_cast<int>(key - sf::Keyboard::Num1);
                    state.camera = 0;
                    state.depthTest = true;
                    printState(state);
                } else if (key == sf::Keyboard::C && state.task >= 2) {
                    state.camera = (state.camera + 1) % CAMERA_COUNT;
                    printState(state);
                } else if (key == sf::Keyboard::D && state.task >= 4) {
                    state.depthTest = !state.depthTest;
                    printState(state);
                } else if (key == sf::Keyboard::N) {
                    state.negativeAxes = !state.negativeAxes;
                }
            }
        }
        if (!window.isOpen()) break;

        drawScene(state);
        window.display();
    }
    return 0;
}
