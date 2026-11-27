#pragma once
// mission.hpp - stan misji i logika: sterowanie dronem, kolizje, inspekcja stanowisk, restart.
// Ten modul nie rysuje niczego - hangar.cpp i hud_view.cpp tylko czytaja struktury Mission.

#include <array>

#include "g3d/collision.hpp"
#include "g3d/math.hpp"

namespace mp {

const float FLOOR_Y = -0.25f;
const int STATION_COUNT = 3;

struct Station {
    const char* label;
    float x, z;
};
extern const std::array<Station, STATION_COUNT> STATIONS;

// Parametry rozgrywki w jednym miejscu.
namespace tuning {
const float MAX_SPEED = 2.2f;          // predkosc pozioma [j/s]
const float LIFT_SPEED = 1.3f;         // predkosc pionowa [j/s]
const float TURN_SPEED = 2.2f;         // obrot [rad/s]
const float ACCELERATION = 5.0f;       // jak szybko predkosc dazy do zadanej [1/s]
const float SCAN_RADIUS = 1.10f;       // strefa inspekcji: kula wokol latarni stanowiska
const float SCAN_TIME = 1.4f;          // czas trzymania Spacji potrzebny do inspekcji [s]
const float SCAN_DECAY = 0.8f;         // spadek postepu po puszczeniu Spacji [1/s]
const float HANGAR_HALF = 3.78f;       // wewnetrzna krawedz scian hangaru
const float MIN_ALTITUDE = FLOOR_Y + 0.16f;
const float MAX_ALTITUDE = 2.4f;
const float BEACON_Y = 0.77f;          // srodek kuli-latarni stanowiska
} // namespace tuning

// Sterowanie odczytane z klawiatury: wartosci -1, 0 albo 1.
struct InputState {
    float forward = 0.0f;   // W/S
    float strafe = 0.0f;    // Q/E
    float lift = 0.0f;      // R/F
    float turn = 0.0f;      // A/D (dodatnio = w lewo)
    bool scan = false;      // Spacja trzymana
};

struct Drone {
    g3d::Vec3 position;
    g3d::Vec3 velocity;
    float heading = 0.0f;          // radiany, 0 = nos w strone -Z
    float yawRate = 0.0f;
    float tiltForward = 0.0f;      // stopnie, efekt wizualny (pochylenie przy lataniu)
    float tiltRight = 0.0f;
    float rotorAngle = 0.0f;       // stopnie
    float rotorSpeed = 900.0f;     // stopnie/s, zalezy od ciagu
    float collisionFlash = 0.0f;   // > 0: dron swieci na czerwono po uderzeniu
};

enum class EventKind { None, Started, Inspected, AlreadyInspected, OutOfRange, Collision, MissionComplete };

struct MissionEvent {
    EventKind kind = EventKind::None;
    int station = -1;
    float age = 1.0e9f;            // sekundy od zdarzenia (HUD wygasza komunikat)
};

struct Mission {
    Drone drone;
    std::array<bool, STATION_COUNT> inspected;
    std::array<float, STATION_COUNT> inspectedAt;      // Mission::time - do efektu fali
    std::array<float, STATION_COUNT> inspectedClock;   // Mission::missionTime - do HUD
    int zoneStation = -1;          // stanowisko, w ktorego strefie jest dron (-1 = zadne)
    int progressStation = -1;      // stanowisko, ktorego dotyczy scanProgress
    float scanProgress = 0.0f;     // 0..1
    bool scanning = false;         // w tej klatce trwa skanowanie
    float time = 0.0f;             // czas animacji (stoi w pauzie)
    float missionTime = 0.0f;      // stoper misji (zatrzymuje sie po ukonczeniu)
    bool finished = false;
    float finishedAt = 0.0f;       // Mission::time w chwili ukonczenia
    MissionEvent event;

    Mission() { inspected.fill(false); inspectedAt.fill(0.0f); inspectedClock.fill(0.0f); }
    int inspectedCount() const;
};

g3d::Vec3 beaconPosition(int station);
float distanceToBeacon(const Mission& m, int station);
int nearestPendingStation(const Mission& m);         // -1, gdy wszystko sprawdzone
g3d::Vec3 droneForward(float heading);
g3d::Vec3 droneRight(float heading);

InputState readKeyboard();
void resetMission(Mission& m);                       // restart bez zamykania programu
void onScanPressed(Mission& m);                      // komunikat, gdy Spacja nic nie zmieni
void updateMission(Mission& m, const InputState& input, float dt);

} // namespace mp
