#include "mission.hpp"

#include <SFML/Window/Keyboard.hpp>

#include <cmath>
#include <iostream>
#include <vector>

namespace mp {

const std::array<Station, STATION_COUNT> STATIONS = {{
    {"ALFA", -2.05f, -1.55f},
    {"BETA", 2.00f, -1.40f},
    {"GAMMA", 1.70f, 2.05f},
}};

namespace {

using g3d::Vec3;

const Vec3 DRONE_START(0.0f, 0.72f, 0.0f);
const Vec3 DRONE_HALF(0.58f, 0.14f, 0.58f);   // AABB niezalezne od obrotu (ramiona + wirniki)

// Przeszkody = te same wymiary co w drawStation(): podstawa, kolumna, latarnia.
std::vector<g3d::Aabb> buildObstacles() {
    std::vector<g3d::Aabb> list;
    for (const Station& s : STATIONS) {
        list.push_back(g3d::Aabb::around(Vec3(s.x, -0.10f, s.z), Vec3(0.45f, 0.15f, 0.45f)));
        list.push_back(g3d::Aabb::around(Vec3(s.x, 0.22f, s.z), Vec3(0.15f, 0.325f, 0.15f)));
        list.push_back(g3d::Aabb::around(Vec3(s.x, tuning::BEACON_Y, s.z), Vec3(0.22f, 0.22f, 0.22f)));
    }
    return list;
}

const std::vector<g3d::Aabb>& obstacles() {
    static const std::vector<g3d::Aabb> list = buildObstacles();
    return list;
}

// Suma wnikniec drona w przeszkody; slideMove odrzuca ruch w osi, ktory ja zwieksza.
float depthAt(const Vec3& p) {
    const g3d::Aabb box = g3d::Aabb::around(p, DRONE_HALF);
    float sum = 0.0f;
    for (const g3d::Aabb& o : obstacles()) sum += g3d::penetration(box, o);
    return sum;
}

void raiseEvent(Mission& m, EventKind kind, int station) {
    m.event.kind = kind;
    m.event.station = station;
    m.event.age = 0.0f;
    const char* name = station >= 0 ? STATIONS[static_cast<std::size_t>(station)].label : "";
    switch (kind) {
    case EventKind::Started: std::cout << "[misja] Start: sprawdz ALFA, BETA i GAMMA.\n"; break;
    case EventKind::Inspected:
        std::cout << "[misja] " << name << " sprawdzone (" << m.inspectedCount() << "/" << STATION_COUNT << ")\n";
        break;
    case EventKind::AlreadyInspected: std::cout << "[misja] " << name << " bylo juz sprawdzone.\n"; break;
    case EventKind::OutOfRange: std::cout << "[misja] Za daleko od stanowiska.\n"; break;
    case EventKind::Collision: std::cout << "[misja] Kolizja.\n"; break;
    case EventKind::MissionComplete:
        std::cout << "[misja] MISJA ZAKONCZONA w " << m.missionTime << " s. Enter = nowa misja.\n";
        break;
    case EventKind::None: break;
    }
}

float axis(bool negative, bool positive) { return (positive ? 1.0f : 0.0f) - (negative ? 1.0f : 0.0f); }

void updateDrone(Mission& m, const InputState& in, float dt) {
    using namespace tuning;
    Drone& d = m.drone;

    // Obrot i predkosc dazace do wartosci zadanych - ruch jest plynny i niezalezny od FPS.
    d.yawRate = g3d::smoothTowards(d.yawRate, in.turn * TURN_SPEED, 10.0f, dt);
    d.heading = g3d::angleDelta(0.0f, d.heading + d.yawRate * dt);

    const Vec3 forward = droneForward(d.heading), right = droneRight(d.heading);
    Vec3 desired = forward * (in.forward * MAX_SPEED) + right * (in.strafe * MAX_SPEED);
    desired.y = in.lift * LIFT_SPEED;
    d.velocity.x = g3d::smoothTowards(d.velocity.x, desired.x, ACCELERATION, dt);
    d.velocity.y = g3d::smoothTowards(d.velocity.y, desired.y, ACCELERATION, dt);
    d.velocity.z = g3d::smoothTowards(d.velocity.z, desired.z, ACCELERATION, dt);

    // Ruch os po osi: zablokowana os stoi, pozostale dzialaja (dron slizga sie po przeszkodzie).
    bool blocked[3];
    Vec3 next = g3d::slideMove(d.position, d.velocity * dt, depthAt, blocked);
    const float limit = HANGAR_HALF - DRONE_HALF.x;
    const Vec3 unclamped = next;
    next.x = g3d::clampf(next.x, -limit, limit);
    next.z = g3d::clampf(next.z, -limit, limit);
    next.y = g3d::clampf(next.y, MIN_ALTITUDE, MAX_ALTITUDE);
    blocked[0] = blocked[0] || next.x != unclamped.x;
    blocked[1] = blocked[1] || next.y != unclamped.y;
    blocked[2] = blocked[2] || next.z != unclamped.z;

    bool hit = false;
    float* velocity[3] = {&d.velocity.x, &d.velocity.y, &d.velocity.z};
    for (int a = 0; a < 3; ++a) {
        if (!blocked[a]) continue;
        // Podloga i sufit to nie kolizja - tylko sciany i stanowiska.
        if (a != 1 && std::fabs(*velocity[a]) > 0.25f) hit = true;
        *velocity[a] = 0.0f;
    }
    d.position = next;

    if (hit) {
        if (d.collisionFlash <= 0.0f) raiseEvent(m, EventKind::Collision, -1);
        d.collisionFlash = 0.35f;
    }
    d.collisionFlash = std::max(0.0f, d.collisionFlash - dt);

    // Efekty: pochylenie w kierunku lotu, szybsze wirniki przy wiekszym ciagu.
    const float vf = g3d::dot(d.velocity, forward) / MAX_SPEED;
    const float vr = g3d::dot(d.velocity, right) / MAX_SPEED;
    d.tiltForward = g3d::smoothTowards(d.tiltForward, 16.0f * vf, 6.0f, dt);
    d.tiltRight = g3d::smoothTowards(d.tiltRight, 16.0f * vr - 7.0f * d.yawRate / TURN_SPEED, 6.0f, dt);
    const float thrust = g3d::clampf(g3d::length(d.velocity) / MAX_SPEED + std::max(0.0f, d.velocity.y), 0.0f, 1.5f);
    d.rotorSpeed = g3d::smoothTowards(d.rotorSpeed, 900.0f + 1100.0f * thrust, 4.0f, dt);
    d.rotorAngle = std::fmod(d.rotorAngle + d.rotorSpeed * dt, 360.0f);
}

void updateInspection(Mission& m, const InputState& in, float dt) {
    using namespace tuning;
    // Ktore stanowisko jest w zasiegu? Strefy nie nachodza na siebie, ale bierzemy najblizsze.
    int zone = -1;
    float best = SCAN_RADIUS;
    for (int i = 0; i < STATION_COUNT; ++i) {
        const float dist = distanceToBeacon(m, i);
        if (dist < best) { best = dist; zone = i; }
    }
    m.zoneStation = zone;
    if (zone != m.progressStation) {      // postep nie przechodzi na inne stanowisko
        m.progressStation = zone;
        m.scanProgress = 0.0f;
    }
    m.scanning = false;
    if (zone < 0 || m.finished) return;

    const std::size_t i = static_cast<std::size_t>(zone);
    if (m.inspected[i]) {                 // drugi raz nie liczymy
        m.scanProgress = 0.0f;
        return;
    }
    if (in.scan) {
        m.scanning = true;
        m.scanProgress += dt / SCAN_TIME;
    } else {
        m.scanProgress = std::max(0.0f, m.scanProgress - SCAN_DECAY * dt);
    }
    if (m.scanProgress < 1.0f) return;

    m.inspected[i] = true;
    m.inspectedAt[i] = m.time;
    m.inspectedClock[i] = m.missionTime;
    m.scanProgress = 0.0f;
    m.scanning = false;
    raiseEvent(m, EventKind::Inspected, zone);
    if (m.inspectedCount() == STATION_COUNT) {
        m.finished = true;
        m.finishedAt = m.time;
        raiseEvent(m, EventKind::MissionComplete, -1);
    }
}

} // namespace

int Mission::inspectedCount() const {
    int count = 0;
    for (bool done : inspected) count += done ? 1 : 0;
    return count;
}

g3d::Vec3 beaconPosition(int station) {
    const Station& s = STATIONS[static_cast<std::size_t>(station)];
    return g3d::Vec3(s.x, tuning::BEACON_Y, s.z);
}

float distanceToBeacon(const Mission& m, int station) {
    return g3d::length(m.drone.position - beaconPosition(station));
}

int nearestPendingStation(const Mission& m) {
    int nearest = -1;
    float best = 1.0e9f;
    for (int i = 0; i < STATION_COUNT; ++i) {
        if (m.inspected[static_cast<std::size_t>(i)]) continue;
        const float dist = distanceToBeacon(m, i);
        if (dist < best) { best = dist; nearest = i; }
    }
    return nearest;
}

// glRotatef(heading, 0, 1, 0) obraca nos (0, 0, -1) na (-sin h, 0, -cos h).
g3d::Vec3 droneForward(float heading) { return g3d::Vec3(-std::sin(heading), 0.0f, -std::cos(heading)); }
g3d::Vec3 droneRight(float heading) { return g3d::Vec3(std::cos(heading), 0.0f, -std::sin(heading)); }

InputState readKeyboard() {
    using K = sf::Keyboard;
    InputState in;
    in.forward = axis(K::isKeyPressed(K::S) || K::isKeyPressed(K::Down), K::isKeyPressed(K::W) || K::isKeyPressed(K::Up));
    in.turn = axis(K::isKeyPressed(K::D) || K::isKeyPressed(K::Right), K::isKeyPressed(K::A) || K::isKeyPressed(K::Left));
    in.strafe = axis(K::isKeyPressed(K::Q), K::isKeyPressed(K::E));
    in.lift = axis(K::isKeyPressed(K::F) || K::isKeyPressed(K::PageDown),
                   K::isKeyPressed(K::R) || K::isKeyPressed(K::PageUp));
    in.scan = K::isKeyPressed(K::Space);
    return in;
}

void resetMission(Mission& m) {
    m = Mission();
    m.drone.position = DRONE_START;
    raiseEvent(m, EventKind::Started, -1);
}

void onScanPressed(Mission& m) {
    if (m.finished) return;
    if (m.zoneStation < 0) raiseEvent(m, EventKind::OutOfRange, nearestPendingStation(m));
    else if (m.inspected[static_cast<std::size_t>(m.zoneStation)]) raiseEvent(m, EventKind::AlreadyInspected, m.zoneStation);
}

void updateMission(Mission& m, const InputState& input, float dt) {
    m.time += dt;
    if (!m.finished) m.missionTime += dt;
    m.event.age += dt;
    updateDrone(m, input, dt);
    updateInspection(m, input, dt);
}

} // namespace mp
