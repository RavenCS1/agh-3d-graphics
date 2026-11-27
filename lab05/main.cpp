// ============================================================================
// Grafika 3D - Laboratorium 5: teksturowanie i pierwsze programy GLSL
// Zadania 17-20 w jednym programie. C++11+, SFML 2.5.1, OpenGL 2.1 + GLU, GLSL 1.20.
// Katalog roboczy musi zawierac assets/ oraz shaders/ (VS: Working Directory = $(ProjectDir)).
//
// Sterowanie:
//   F1/F2/F3/F4    stan po zadaniu 17/18/19/20 (start: zadanie 20)
//   LPM + mysz     obrot kamery, kolko - promien; strzalki / PgUp PgDn - to samo z klawiatury
//   N              reset kamery;  9 / 0, V / B  perspektywa / orto, fov
//   Spacja         pauza animacji (zatrzymuje tez pulsowanie i faze GPU)
//   Z / X          amplituda -/+ 0.05, zakres 0-0.6
//   Backspace      reset fali (R z lab. 4 jest teraz zajete przez powtorzenia)
//   L              szkielet / wypelnienie
//   T              tekstura uv_grid.png / stone.png (zad. 18+)
//   R              powtorzenia 1x / 4x (zad. 18+)
//   E              rozszerzenie zad. 18: cykl 1x -> 2x -> 4x -> 8x jednym klawiszem
//   F              filtrowanie nearest / linear (zad. 18+)
//   H / G / P      shader wl./wyl., szarosc, pulsowanie (zad. 19)
//   C              deformacja CPU / GPU (zad. 20)
//   U / I          rozszerzenie zad. 20: czestotliwosc fal -/+ 0.25 (CPU i GPU)
//   F12            zapis klatki do Lab5_capture.png
//   Esc            wyjscie
//
// Odpowiedzi na pytania z zadania 20: ODPOWIEDZI.md (oraz komentarz przy updateGeometry()).
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
const int GRID = 21;                      // 21 x 21 wierzcholkow, 800 trojkatow
const float BASE_Y = -0.35f;
const float WAVE_SPEED = 1.5f;            // radiany fazy na sekunde
const float LIGHT_POSITION[3] = {1.5f, 2.2f, 1.3f};

float clampf(float v, float lo, float hi) { return std::max(lo, std::min(v, hi)); }

// ---------------------------------------------------------------------------
// Wektory (z laboratorium 4)
// ---------------------------------------------------------------------------
struct Vec3 {
    float x, y, z;
    Vec3() : x(0), y(0), z(0) {}
    Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
};
Vec3 operator+(const Vec3& a, const Vec3& b) { return Vec3(a.x + b.x, a.y + b.y, a.z + b.z); }
Vec3 operator-(const Vec3& a, const Vec3& b) { return Vec3(a.x - b.x, a.y - b.y, a.z - b.z); }

Vec3 cross(const Vec3& u, const Vec3& v) {
    return Vec3(u.y * v.z - u.z * v.y, u.z * v.x - u.x * v.z, u.x * v.y - u.y * v.x);
}

Vec3 normalize(const Vec3& v) {
    const float len = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    if (len < 1.0e-6f) return Vec3(0.0f, 1.0f, 0.0f);
    return Vec3(v.x / len, v.y / len, v.z / len);
}

// ============================================================================
// ZADANIE 17 - siatka z wspolrzednymi UV liczonymi raz, przy tworzeniu
// ============================================================================
struct SurfaceMesh {
    std::vector<Vec3> positions;
    std::vector<Vec3> normals;
    std::vector<sf::Vector2f> uv;
    bool flat = false;                    // true = dane plaskiej siatki dla trybu GPU

    SurfaceMesh() : positions(GRID * GRID), normals(GRID * GRID), uv(GRID * GRID) {
        for (int row = 0; row < GRID; ++row) {
            for (int col = 0; col < GRID; ++col) {
                const float x = -1.0f + 0.1f * static_cast<float>(col);
                const float z = -1.0f + 0.1f * static_cast<float>(row);
                // Lewa/prawa krawedz -> u = 0/1, krawedzie z = -1/+1 -> v = 0/1.
                uv[row * GRID + col] = sf::Vector2f((x + 1.0f) / 2.0f, (z + 1.0f) / 2.0f);
            }
        }
    }
};

int index(int row, int col) { return row * GRID + col; }

// Te same trojkaty CCW (patrzac od +Y) co w laboratorium 4: (a, d, c) i (a, c, b).
void cellTriangles(int row, int col, int tri[2][3]) {
    const int a = index(row, col), b = index(row, col + 1);
    const int c = index(row + 1, col + 1), d = index(row + 1, col);
    tri[0][0] = a; tri[0][1] = d; tri[0][2] = c;
    tri[1][0] = a; tri[1][1] = c; tri[1][2] = b;
}

struct Wave {
    float amplitude = 0.30f;
    float time = 0.0f;                    // wspolny czas animacji: faza, pulsowanie, GPU
    float frequency = 1.0f;               // rozszerzenie zad. 20
    bool paused = false;
    float phase() const { return WAVE_SPEED * time; }
};

float waveHeight(float x, float z, const Wave& w) {
    const float k = PI * w.frequency;
    return BASE_Y + w.amplitude * std::sin(k * x - w.phase()) * std::cos(k * z + w.phase());
}

// Tryb CPU: pozycje i normalne jak w zadaniu 15 (iloczyn wektorowy + usrednianie).
void deformOnCpu(SurfaceMesh& m, const Wave& w) {
    for (int row = 0; row < GRID; ++row) {
        for (int col = 0; col < GRID; ++col) {
            const float x = -1.0f + 0.1f * static_cast<float>(col), z = -1.0f + 0.1f * static_cast<float>(row);
            m.positions[index(row, col)] = Vec3(x, waveHeight(x, z, w), z);
        }
    }
    std::fill(m.normals.begin(), m.normals.end(), Vec3());
    for (int row = 0; row < GRID - 1; ++row) {
        for (int col = 0; col < GRID - 1; ++col) {
            int tri[2][3];
            cellTriangles(row, col, tri);
            for (int t = 0; t < 2; ++t) {
                const Vec3& a = m.positions[tri[t][0]];
                const Vec3 n = normalize(cross(m.positions[tri[t][1]] - a, m.positions[tri[t][2]] - a));
                for (int k = 0; k < 3; ++k) m.normals[tri[t][k]] = m.normals[tri[t][k]] + n;
            }
        }
    }
    for (Vec3& n : m.normals) n = normalize(n);
    m.flat = false;
}

// Tryb GPU: plaska siatka y = -0.35 i normalne (0, 1, 0); deformacja dopiero w wave.vert.
void makeFlat(SurfaceMesh& m) {
    for (int row = 0; row < GRID; ++row) {
        for (int col = 0; col < GRID; ++col) {
            m.positions[index(row, col)] = Vec3(-1.0f + 0.1f * static_cast<float>(col), BASE_Y, -1.0f + 0.1f * static_cast<float>(row));
            m.normals[index(row, col)] = Vec3(0.0f, 1.0f, 0.0f);
        }
    }
    m.flat = true;
}

// ---------------------------------------------------------------------------
// Zasoby: tekstury i shadery ladowane raz, przy starcie
// ---------------------------------------------------------------------------
struct Resources {
    sf::Texture uvGrid;                   // ZADANIE 17
    sf::Texture stone;                    // ZADANIE 18
    sf::Shader surfaceShader;             // ZADANIE 19
    sf::Shader waveShader;                // ZADANIE 20
    bool surfaceShaderReady = false;
    bool waveShaderReady = false;
};

bool loadTextures(Resources& r) {
    if (!r.uvGrid.loadFromFile("assets/uv_grid.png")) {
        std::cerr << "Nie mozna wczytac assets/uv_grid.png - sprawdz katalog roboczy.\n";
        return false;
    }
    if (!r.stone.loadFromFile("assets/stone.png")) {
        std::cerr << "Nie mozna wczytac assets/stone.png - sprawdz katalog roboczy.\n";
        return false;
    }
    r.uvGrid.setRepeated(true);           // ZADANIE 18: UV > 1 powtarza obraz
    r.stone.setRepeated(true);
    return true;
}

void loadShaders(Resources& r) {
    if (!sf::Shader::isAvailable()) {
        std::cerr << "Shadery GLSL nie sa dostepne na tym stanowisku - zadania 19-20 bez GLSL.\n";
        return;
    }
    r.surfaceShaderReady = r.surfaceShader.loadFromFile("shaders/surface.vert", "shaders/surface.frag");
    if (!r.surfaceShaderReady) std::cerr << "Blad ladowania shaders/surface.vert + surface.frag (szczegoly wyzej).\n";
    r.waveShaderReady = r.waveShader.loadFromFile("shaders/wave.vert", "shaders/wave.frag");
    if (!r.waveShaderReady) std::cerr << "Blad ladowania shaders/wave.vert + wave.frag (szczegoly wyzej).\n";
}

// ---------------------------------------------------------------------------
// Stan programu
// ---------------------------------------------------------------------------
struct OrbitCamera {
    float radius = 4.2f, azimuth = 0.55f, elevation = 0.62f;
};

struct AppState {
    int task = 20;
    OrbitCamera camera;
    bool perspective = true;
    float fov = 45.0f;
    bool dragging = false;
    sf::Vector2i lastMouse;
    Wave wave;
    bool wireframe = false;
    bool useStone = false;                // ZADANIE 18
    float tileCount = 1.0f;
    bool smoothFilter = true;
    bool shaderEnabled = true;            // ZADANIE 19
    bool grayscale = false;
    bool pulse = false;
    bool gpuWave = true;                  // ZADANIE 20
    bool screenshot = false;
};

// Co faktycznie obowiazuje w danym zadaniu (wczesniejsze zadania nie widza pozniejszych opcji).
float activeTileCount(const AppState& s) { return s.task >= 18 ? s.tileCount : 1.0f; }
bool stoneActive(const AppState& s) { return s.task >= 18 && s.useStone; }
bool surfaceShaderActive(const AppState& s, const Resources& r) {
    return s.task == 19 && s.shaderEnabled && r.surfaceShaderReady;
}
bool waveShaderActive(const AppState& s, const Resources& r) { return s.task == 20 && r.waveShaderReady; }
bool gpuDeformation(const AppState& s, const Resources& r) { return waveShaderActive(s, r) && s.gpuWave; }

// ZADANIE 18: filtr ustawiany na obu teksturach, wiec po T nadal obowiazuje wybrany tryb.
void applyFilter(Resources& r, bool smooth) {
    r.uvGrid.setSmooth(smooth);
    r.stone.setSmooth(smooth);
}

// ---------------------------------------------------------------------------
// Kamera i projekcja
// ---------------------------------------------------------------------------
void setCamera(const OrbitCamera& c) {
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(c.radius * std::cos(c.elevation) * std::sin(c.azimuth),
              c.radius * std::sin(c.elevation),
              c.radius * std::cos(c.elevation) * std::cos(c.azimuth),
              0.0, -0.25, 0.0,  0.0, 1.0, 0.0);
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

// ---------------------------------------------------------------------------
// Rysowanie
// ---------------------------------------------------------------------------
void drawAxes() {
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    glColor3f(1.0f, 0.2f, 0.2f); glVertex3f(0, 0, 0); glVertex3f(1.4f, 0, 0);
    glColor3f(0.2f, 1.0f, 0.2f); glVertex3f(0, 0, 0); glVertex3f(0, 1.4f, 0);
    glColor3f(0.2f, 0.5f, 1.0f); glVertex3f(0, 0, 0); glVertex3f(0, 0, 1.4f);
    glEnd();
    glLineWidth(1.0f);
}

void drawLightMarker() {
    glPointSize(9.0f);
    glBegin(GL_POINTS);
    glColor3f(1.0f, 0.95f, 0.35f);
    glVertex3f(LIGHT_POSITION[0], LIGHT_POSITION[1], LIGHT_POSITION[2]);
    glEnd();
    glPointSize(1.0f);
}

// ZADANIA 17-18: glTexCoord2f przed kazdym glVertex3f; UV * tileCount daje powtorzenia.
void drawSurface(const SurfaceMesh& m, float tileCount, bool wireframe) {
    glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
    glColor3f(1.0f, 1.0f, 1.0f);          // bialy - kolor nie zabarwia tekstury
    glBegin(GL_TRIANGLES);
    for (int row = 0; row < GRID - 1; ++row) {
        for (int col = 0; col < GRID - 1; ++col) {
            int tri[2][3];
            cellTriangles(row, col, tri);
            for (int t = 0; t < 2; ++t) {
                for (int k = 0; k < 3; ++k) {
                    const int i = tri[t][k];
                    glNormal3f(m.normals[i].x, m.normals[i].y, m.normals[i].z);
                    glTexCoord2f(m.uv[i].x * tileCount, m.uv[i].y * tileCount);
                    glVertex3f(m.positions[i].x, m.positions[i].y, m.positions[i].z);
                }
            }
        }
    }
    glEnd();
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void drawScene(const SurfaceMesh& mesh, Resources& r, const AppState& s) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    setCamera(s.camera);
    glDisable(GL_LIGHTING);               // zad. 17-19: bez oswietlenia; zad. 20: oswietlenie w GLSL

    drawAxes();                           // bez tekstury i bez shadera - kolory osi bez zmian
    if (s.task == 20) drawLightMarker();

    const sf::Texture* texture = stoneActive(s) ? &r.stone : &r.uvGrid;
    sf::Texture::bind(texture);
    glEnable(GL_TEXTURE_2D);

    sf::Shader* shader = nullptr;
    if (surfaceShaderActive(s, r)) {      // ZADANIE 19
        r.surfaceShader.setUniform("uTexture", sf::Shader::CurrentTexture);
        r.surfaceShader.setUniform("uTime", s.wave.time);
        r.surfaceShader.setUniform("uGrayscale", s.grayscale ? 1.0f : 0.0f);
        r.surfaceShader.setUniform("uPulse", s.pulse ? 1.0f : 0.0f);
        shader = &r.surfaceShader;
    } else if (waveShaderActive(s, r)) {  // ZADANIE 20
        r.waveShader.setUniform("uTexture", sf::Shader::CurrentTexture);
        r.waveShader.setUniform("uPhase", s.wave.phase());
        r.waveShader.setUniform("uAmplitude", s.wave.amplitude);
        r.waveShader.setUniform("uFrequency", s.wave.frequency);
        r.waveShader.setUniform("uGpuWave", s.gpuWave ? 1.0f : 0.0f);
        r.waveShader.setUniform("uLightWorld",
                                sf::Glsl::Vec3(LIGHT_POSITION[0], LIGHT_POSITION[1], LIGHT_POSITION[2]));
        shader = &r.waveShader;
    }
    sf::Shader::bind(shader);             // tylko na czas rysowania siatki

    drawSurface(mesh, activeTileCount(s), s.wireframe);

    sf::Shader::bind(nullptr);
    sf::Texture::bind(nullptr);
    glDisable(GL_TEXTURE_2D);
}

void saveScreenshot(unsigned width, unsigned height) {
    std::vector<sf::Uint8> pixels(static_cast<std::size_t>(width) * height * 4u);
    glReadBuffer(GL_BACK);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height),
                 GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    sf::Image image;
    image.create(width, height, pixels.data());
    image.flipVertically();
    std::cout << (image.saveToFile("Lab5_capture.png") ? "Zapisano Lab5_capture.png\n"
                                                         : "Blad zapisu Lab5_capture.png\n");
}

// ---------------------------------------------------------------------------
// ZADANIE 20 - aktualizacja geometrii.
// CPU: wysokosc i normalne liczone tutaj, w C++, dla kazdego z 441 wierzcholkow w kazdej klatce.
// GPU: C++ wysyla niezmienna plaska siatke, a wysokosc i normalna liczy wave.vert;
//      co klatke zmieniaja sie tylko uniformy uPhase / uAmplitude / uFrequency.
// ---------------------------------------------------------------------------
void updateGeometry(SurfaceMesh& mesh, const AppState& s, const Resources& r) {
    if (gpuDeformation(s, r)) {
        if (!mesh.flat) makeFlat(mesh);   // plaska siatka tylko raz, przy przelaczeniu
    } else {
        deformOnCpu(mesh, s.wave);
    }
}

// ---------------------------------------------------------------------------
// Sterowanie
// ---------------------------------------------------------------------------
void updateHeldKeys(AppState& s, float dt) {
    using K = sf::Keyboard;
    OrbitCamera& c = s.camera;
    if (K::isKeyPressed(K::Left)) c.azimuth -= 1.2f * dt;
    if (K::isKeyPressed(K::Right)) c.azimuth += 1.2f * dt;
    if (K::isKeyPressed(K::Up)) c.elevation += 1.2f * dt;
    if (K::isKeyPressed(K::Down)) c.elevation -= 1.2f * dt;
    if (K::isKeyPressed(K::PageUp)) c.radius -= 2.0f * dt;
    if (K::isKeyPressed(K::PageDown)) c.radius += 2.0f * dt;
    c.elevation = clampf(c.elevation, -1.4f, 1.4f);
    c.radius = clampf(c.radius, 2.5f, 10.0f);
}

void handleMouse(const sf::Event& e, AppState& s) {
    if (e.type == sf::Event::MouseButtonPressed && e.mouseButton.button == sf::Mouse::Left) {
        s.dragging = true;
        s.lastMouse = sf::Vector2i(e.mouseButton.x, e.mouseButton.y);
    } else if (e.type == sf::Event::MouseButtonReleased && e.mouseButton.button == sf::Mouse::Left) {
        s.dragging = false;
    } else if (e.type == sf::Event::MouseMoved && s.dragging) {
        s.camera.azimuth += 0.006f * static_cast<float>(e.mouseMove.x - s.lastMouse.x);
        s.camera.elevation = clampf(s.camera.elevation - 0.006f * static_cast<float>(e.mouseMove.y - s.lastMouse.y), -1.4f, 1.4f);
        s.lastMouse = sf::Vector2i(e.mouseMove.x, e.mouseMove.y);
    } else if (e.type == sf::Event::MouseWheelScrolled && e.mouseWheelScroll.wheel == sf::Mouse::VerticalWheel) {
        s.camera.radius = clampf(s.camera.radius - 0.35f * e.mouseWheelScroll.delta, 2.5f, 10.0f);
    }
}

float nextTileCount(float current) {        // rozszerzenie: 1 -> 2 -> 4 -> 8 -> 1
    if (current < 1.5f) return 2.0f;
    if (current < 3.0f) return 4.0f;
    if (current < 6.0f) return 8.0f;
    return 1.0f;
}

bool handleKeyPressed(sf::Keyboard::Key key, AppState& s, Resources& r) {
    switch (key) {
    case sf::Keyboard::F1: case sf::Keyboard::F2: case sf::Keyboard::F3: case sf::Keyboard::F4:
        s.task = 17 + static_cast<int>(key - sf::Keyboard::F1);
        std::cout << "Zadanie " << s.task << '\n';
        break;
    case sf::Keyboard::N: s.camera = OrbitCamera(); break;
    case sf::Keyboard::Num9: s.perspective = true; return true;
    case sf::Keyboard::Num0: s.perspective = false; return true;
    case sf::Keyboard::V: if (s.perspective) { s.fov = clampf(s.fov - 5.0f, 25.0f, 90.0f); return true; } break;
    case sf::Keyboard::B: if (s.perspective) { s.fov = clampf(s.fov + 5.0f, 25.0f, 90.0f); return true; } break;
    case sf::Keyboard::Space: s.wave.paused = !s.wave.paused; break;
    case sf::Keyboard::Z: s.wave.amplitude = clampf(s.wave.amplitude - 0.05f, 0.0f, 0.60f); break;
    case sf::Keyboard::X: s.wave.amplitude = clampf(s.wave.amplitude + 0.05f, 0.0f, 0.60f); break;
    case sf::Keyboard::BackSpace: s.wave = Wave(); break;
    case sf::Keyboard::L: s.wireframe = !s.wireframe; break;
    case sf::Keyboard::T: if (s.task >= 18) s.useStone = !s.useStone; break;
    case sf::Keyboard::R: if (s.task >= 18) s.tileCount = (s.tileCount == 1.0f) ? 4.0f : 1.0f; break;
    case sf::Keyboard::E: if (s.task >= 18) s.tileCount = nextTileCount(s.tileCount); break;
    case sf::Keyboard::F:
        if (s.task >= 18) { s.smoothFilter = !s.smoothFilter; applyFilter(r, s.smoothFilter); }
        break;
    case sf::Keyboard::H: if (s.task == 19) s.shaderEnabled = !s.shaderEnabled; break;
    case sf::Keyboard::G: if (s.task == 19) s.grayscale = !s.grayscale; break;
    case sf::Keyboard::P: if (s.task == 19) s.pulse = !s.pulse; break;
    case sf::Keyboard::C: if (s.task == 20) s.gpuWave = !s.gpuWave; break;
    case sf::Keyboard::U: if (s.task == 20) s.wave.frequency = clampf(s.wave.frequency - 0.25f, 0.5f, 3.0f); break;
    case sf::Keyboard::I: if (s.task == 20) s.wave.frequency = clampf(s.wave.frequency + 0.25f, 0.5f, 3.0f); break;
    case sf::Keyboard::F12: s.screenshot = true; break;
    default: break;
    }
    return false;
}

std::string makeTitle(const AppState& s, const Resources& r) {
    std::ostringstream t;
    t << std::fixed << std::setprecision(2) << "Grafika 3D - Lab 5 - zadanie " << s.task
      << " | amplituda " << s.wave.amplitude << (s.wave.paused ? " (pauza)" : "");
    if (s.task >= 18)
        t << " | " << (s.useStone ? "stone" : "uv_grid") << ' ' << static_cast<int>(s.tileCount) << 'x'
          << ", " << (s.smoothFilter ? "linear" : "nearest");
    if (s.task == 19) {
        if (!r.surfaceShaderReady) t << " | shader niedostepny";
        else t << " | shader " << (s.shaderEnabled ? "ON" : "OFF") << ", szarosc " << (s.grayscale ? "ON" : "OFF")
               << ", puls " << (s.pulse ? "ON" : "OFF");
    }
    if (s.task == 20) {
        if (!r.waveShaderReady) t << " | wave shader niedostepny (CPU)";
        else t << " | deformacja " << (s.gpuWave ? "GPU" : "CPU") << ", czestotliwosc " << s.wave.frequency;
    }
    return t.str();
}

} // namespace

int main() {
    const sf::ContextSettings settings(24, 0, 0, 2, 1);
    sf::RenderWindow window(sf::VideoMode(1000, 700), "Grafika 3D - Lab 5", sf::Style::Default, settings);
    window.setVerticalSyncEnabled(true);
    window.setKeyRepeatEnabled(false);
    window.setActive(true);

    Resources resources;
    if (!loadTextures(resources)) return 1;   // ZADANIE 17: bez tekstury nie rysujemy
    loadShaders(resources);

    AppState state;
    applyFilter(resources, state.smoothFilter);

    glClearColor(0.10f, 0.14f, 0.20f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glShadeModel(GL_SMOOTH);
    configureProjection(window.getSize().x, window.getSize().y, state);

    std::cout << "F1-F4 zadania 17-20 | mysz/strzalki/PgUp/PgDn kamera, N reset | 9/0 V/B projekcja\n"
              << "Spacja pauza, Z/X amplituda, Backspace reset fali, L szkielet\n"
              << "T tekstura, R 1x/4x, E 1/2/4/8x, F filtr | H shader, G szarosc, P puls | C CPU/GPU, U/I czestotliwosc\n"
              << "F12 zrzut ekranu | Esc wyjscie\n";

    SurfaceMesh mesh;
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
                else projectionChanged |= handleKeyPressed(event.key.code, state, resources);
            }
            handleMouse(event, state);
        }
        if (!window.isOpen()) break;

        // 1) parametry animacji, 2) dane siatki, 3) rysowanie
        updateHeldKeys(state, dt);
        if (!state.wave.paused) state.wave.time += dt;   // jeden czas dla CPU, GPU i pulsowania
        if (projectionChanged) configureProjection(window.getSize().x, window.getSize().y, state);
        updateGeometry(mesh, state, resources);

        const std::string title = makeTitle(state, resources);
        if (title != lastTitle) { window.setTitle(title); lastTitle = title; }

        drawScene(mesh, resources, state);
        if (state.screenshot) {
            saveScreenshot(window.getSize().x, window.getSize().y);
            state.screenshot = false;
        }
        window.display();
    }
    return 0;
}
