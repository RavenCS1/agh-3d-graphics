#pragma once
// g3d/screenshot.hpp - zapis aktualnej klatki do PNG (lab 4, rozszerzenie zad. 16).
// Wywolaj po narysowaniu sceny, a PRZED window.display() - czytamy tylny bufor.

#include <iostream>
#include <string>
#include <vector>

#include "gl.hpp"

namespace g3d {

inline bool saveScreenshot(unsigned width, unsigned height, const std::string& path) {
    if (width == 0 || height == 0) return false;
    std::vector<sf::Uint8> rgba(static_cast<std::size_t>(width) * height * 4u);
    glReadBuffer(GL_BACK);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height),
                 GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
    sf::Image image;
    image.create(width, height, rgba.data());
    image.flipVertically();               // OpenGL zaczyna od dolnego wiersza
    const bool ok = image.saveToFile(path);
    std::cout << (ok ? "Zapisano " : "Blad zapisu ") << path << '\n';
    return ok;
}

} // namespace g3d
