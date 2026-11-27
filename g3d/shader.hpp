#pragma once
// g3d/shader.hpp - shadery GLSL 1.20 przez sf::Shader (lab 5, zad. 19-20).
// SFML sam laduje funkcje OpenGL potrzebne do shaderow - nie trzeba GLEW ani GLAD.
//
//   sf::Shader wave;
//   if (!g3d::loadShader(wave, "shaders/wave.vert", "shaders/wave.frag")) ...
//   wave.setUniform("uTexture", sf::Shader::CurrentTexture);  // tekstura z jednostki 0
//   wave.setUniform("uPhase", phase);
//   {
//       g3d::ScopedShader use(&wave);    // shader tylko na czas rysowania tej siatki
//       mesh.draw();
//   }

#include <iostream>
#include <string>

#include "gl.hpp"

namespace g3d {

inline bool loadShader(sf::Shader& shader, const std::string& vertexPath, const std::string& fragmentPath) {
    if (!sf::Shader::isAvailable()) {
        std::cerr << "Shadery GLSL nie sa dostepne na tym komputerze.\n";
        return false;
    }
    if (!shader.loadFromFile(vertexPath, fragmentPath)) {
        // SFML wypisuje wyzej szczegoly bledu kompilacji GLSL.
        std::cerr << "Nie udalo sie zaladowac shaderow: " << vertexPath << ", " << fragmentPath << '\n';
        return false;
    }
    return true;
}

class ScopedShader {
public:
    explicit ScopedShader(const sf::Shader* shader) { sf::Shader::bind(shader); }
    ~ScopedShader() { sf::Shader::bind(nullptr); }
    ScopedShader(const ScopedShader&) = delete;
    ScopedShader& operator=(const ScopedShader&) = delete;
};

} // namespace g3d
