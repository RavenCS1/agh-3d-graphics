#pragma once
// g3d/mesh.hpp - regularna siatka trojkatow z normalnymi i UV (lab 4, zad. 13-15; lab 5, zad. 17).
//
//   g3d::GridMesh grid(21, 1.0f);                         // 21x21 wierzcholkow, x,z w [-1,1]
//   grid.build([&](float x, float z) { return -0.35f + 0.3f * std::sin(g3d::PI * x); });
//   grid.draw(true /*smooth*/, 1.0f /*powtorzenia UV*/);
//   grid.drawNormals(0.15f, 4);
//
// Indeks wierzcholka: row * side + col; wiersz biegnie wzdluz Z, kolumna wzdluz X.
// Kazda komorka to dwa trojkaty (a, d, c) i (a, c, b), gdzie
//   a = (row, col), b = (row, col+1), c = (row+1, col+1), d = (row+1, col).
// Ich normalna geometryczna cross(d-a, c-a) wskazuje +Y, czyli patrzac z gory wierzcholki
// sa ulozone przeciwnie do ruchu wskazowek zegara (CCW, domyslny przod w OpenGL).

#include <vector>

#include "gl.hpp"
#include "math.hpp"

namespace g3d {

class GridMesh {
public:
    explicit GridMesh(int verticesPerSide = 21, float halfSize = 1.0f)
        : side_(verticesPerSide), half_(halfSize),
          positions_(static_cast<std::size_t>(verticesPerSide * verticesPerSide)),
          normals_(positions_.size()), u_(positions_.size()), v_(positions_.size()) {
        for (int row = 0; row < side_; ++row) {
            for (int col = 0; col < side_; ++col) {
                const std::size_t i = index(row, col);
                // UV liczone raz, przy tworzeniu siatki: (wspolrzedna + 1) / 2
                u_[i] = static_cast<float>(col) / static_cast<float>(side_ - 1);
                v_[i] = static_cast<float>(row) / static_cast<float>(side_ - 1);
                positions_[i] = Vec3(-half_ + 2.0f * half_ * u_[i], 0.0f, -half_ + 2.0f * half_ * v_[i]);
            }
        }
    }

    int side() const { return side_; }
    int cells() const { return side_ - 1; }
    std::size_t index(int row, int col) const { return static_cast<std::size_t>(row * side_ + col); }
    const std::vector<Vec3>& positions() const { return positions_; }
    const std::vector<Vec3>& normals() const { return normals_; }

    // Ustawia wysokosci y = height(x, z) i przelicza normalne wierzcholkowe.
    template <class HeightFn>
    void build(HeightFn height) {
        for (Vec3& p : positions_) p.y = height(p.x, p.z);
        computeVertexNormals();
    }

    // Normalna wierzcholka = znormalizowana suma normalnych sasiednich trojkatow.
    void computeVertexNormals() {
        for (Vec3& n : normals_) n = Vec3();
        for (int row = 0; row < cells(); ++row) {
            for (int col = 0; col < cells(); ++col) {
                std::size_t t[2][3];
                cellTriangles(row, col, t);
                for (int k = 0; k < 2; ++k) {
                    const Vec3 n = faceNormal(t[k][0], t[k][1], t[k][2]);
                    for (int j = 0; j < 3; ++j) normals_[t[k][j]] += n;
                }
            }
        }
        for (Vec3& n : normals_) n = normalize(n);
    }

    Vec3 faceNormal(std::size_t a, std::size_t b, std::size_t c) const {
        return normalize(cross(positions_[b] - positions_[a], positions_[c] - positions_[a]));
    }

    void cellTriangles(int row, int col, std::size_t out[2][3]) const {
        const std::size_t a = index(row, col), b = index(row, col + 1);
        const std::size_t c = index(row + 1, col + 1), d = index(row + 1, col);
        out[0][0] = a; out[0][1] = d; out[0][2] = c;
        out[1][0] = a; out[1][1] = c; out[1][2] = b;
    }

    // smooth = true: normalne wierzcholkowe (cieniowanie gladkie),
    // smooth = false: jedna normalna trojkata dla jego trzech wierzcholkow (flat).
    // uvScale > 1 powtarza teksture (wymaga sf::Texture::setRepeated(true)).
    void draw(bool smooth = true, float uvScale = 1.0f) const {
        glBegin(GL_TRIANGLES);
        for (int row = 0; row < cells(); ++row) {
            for (int col = 0; col < cells(); ++col) {
                std::size_t t[2][3];
                cellTriangles(row, col, t);
                for (int k = 0; k < 2; ++k) {
                    const Vec3 fn = faceNormal(t[k][0], t[k][1], t[k][2]);
                    for (int j = 0; j < 3; ++j) {
                        const std::size_t i = t[k][j];
                        const Vec3& n = smooth ? normals_[i] : fn;
                        glNormal3f(n.x, n.y, n.z);
                        glTexCoord2f(u_[i] * uvScale, v_[i] * uvScale);
                        glVertex3f(positions_[i].x, positions_[i].y, positions_[i].z);
                    }
                }
            }
        }
        glEnd();
    }

    // Odcinki normalnych dla co every-tego wierzcholka (wywoluj przy wylaczonym GL_LIGHTING).
    void drawNormals(float length = 0.15f, int every = 4) const {
        glBegin(GL_LINES);
        for (std::size_t i = 0; i < positions_.size(); i += static_cast<std::size_t>(every)) {
            const Vec3 tip = positions_[i] + normals_[i] * length;
            glVertex3f(positions_[i].x, positions_[i].y, positions_[i].z);
            glVertex3f(tip.x, tip.y, tip.z);
        }
        glEnd();
    }

private:
    int side_;
    float half_;
    std::vector<Vec3> positions_, normals_;
    std::vector<float> u_, v_;
};

} // namespace g3d
