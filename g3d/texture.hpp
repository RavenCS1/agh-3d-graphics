#pragma once
// g3d/texture.hpp - wczytywanie i wiazanie tekstur SFML w klasycznym OpenGL (lab 5, zad. 17-18).
//
//   sf::Texture grid;
//   if (!g3d::loadTexture(grid, "assets/uv_grid.png")) return 1;
//   {
//       g3d::ScopedTexture use(&grid);   // bind + glEnable(GL_TEXTURE_2D)
//       glColor3f(1, 1, 1);              // bialy - kolor nie zabarwia tekstury
//       ... glTexCoord2f + glVertex3f ...
//   }                                    // odwiazanie + glDisable(GL_TEXTURE_2D)

#include <iostream>
#include <string>

#include "gl.hpp"

namespace g3d {

// repeated: UV poza [0,1] powtarza obraz; smooth: filtrowanie liniowe zamiast najblizszego sasiada.
inline bool loadTexture(sf::Texture& texture, const std::string& path,
                        bool repeated = true, bool smooth = true) {
    if (!texture.loadFromFile(path)) {
        std::cerr << "Nie udalo sie wczytac tekstury: " << path
                  << " (sprawdz katalog roboczy programu)\n";
        return false;
    }
    texture.setRepeated(repeated);
    texture.setSmooth(smooth);
    return true;
}

class ScopedTexture {
public:
    explicit ScopedTexture(const sf::Texture* texture) {
        sf::Texture::bind(texture, sf::Texture::Normalized);   // UV w zakresie 0..1
        glEnable(GL_TEXTURE_2D);
    }
    ~ScopedTexture() {
        sf::Texture::bind(nullptr);
        glDisable(GL_TEXTURE_2D);
    }
    ScopedTexture(const ScopedTexture&) = delete;
    ScopedTexture& operator=(const ScopedTexture&) = delete;
};

} // namespace g3d
