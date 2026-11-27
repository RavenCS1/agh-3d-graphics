#pragma once
// g3d/hud.hpp - nakladka 2D (tekst, panele, paski postepu) rysowana przez SFML
// na wierzchu sceny zbudowanej w klasycznym OpenGL.
//
//   g3d::Hud hud;
//   hud.loadFont("assets/fonts/DejaVuSans.ttf");
//   ... drawScene() w OpenGL ...
//   hud.begin(window);                  // pushGLStates: SFML nie psuje stanu OpenGL sceny
//   hud.text(L"Sprawdzone: 2/3", 16, 12, 20, sf::Color::White);
//   hud.bar(16, 40, 200, 10, 0.66f, sf::Color::Green);
//   hud.end(window);                    // popGLStates
//   window.display();
//
// Polskie znaki: uzywaj literalow L"..." (plik zrodlowy w UTF-8 z BOM albo /utf-8 w MSVC).

#include <cmath>
#include <string>

#include "gl.hpp"

namespace g3d {

class Hud {
public:
    bool loadFont(const std::string& path) {
        fontOk_ = font_.loadFromFile(path);
        return fontOk_;
    }
    bool hasFont() const { return fontOk_; }
    const sf::Font& font() const { return font_; }

    void begin(sf::RenderWindow& window) {
        window_ = &window;
        window.pushGLStates();
        const sf::Vector2u size = window.getSize();
        // Uklad wspolrzednych w pikselach okna (takze po zmianie jego rozmiaru).
        window.setView(sf::View(sf::FloatRect(0.0f, 0.0f, static_cast<float>(size.x), static_cast<float>(size.y))));
    }

    void end(sf::RenderWindow& window) {
        window.popGLStates();
        window_ = nullptr;
    }

    float width() const { return window_ ? static_cast<float>(window_->getSize().x) : 0.0f; }
    float height() const { return window_ ? static_cast<float>(window_->getSize().y) : 0.0f; }

    // align: 0 = do lewej, 1 = wysrodkowany, 2 = do prawej (wzgledem x). Zwraca szerokosc tekstu.
    float text(const sf::String& str, float x, float y, unsigned size, sf::Color color,
               int align = 0, bool bold = false, bool shadow = true) {
        if (!window_ || !fontOk_) return 0.0f;
        sf::Text t(str, font_, size);
        t.setStyle(bold ? sf::Text::Bold : sf::Text::Regular);
        const sf::FloatRect b = t.getLocalBounds();
        float px = x;
        if (align == 1) px = x - b.width * 0.5f - b.left;
        if (align == 2) px = x - b.width - b.left;
        if (shadow) {
            t.setFillColor(sf::Color(0, 0, 0, static_cast<sf::Uint8>(color.a * 0.7f)));
            t.setPosition(std::floor(px + 1.5f), std::floor(y + 1.5f));
            window_->draw(t);
        }
        t.setFillColor(color);
        t.setPosition(std::floor(px), std::floor(y));
        window_->draw(t);
        return b.width;
    }

    void panel(float x, float y, float w, float h, sf::Color fill,
               sf::Color outline = sf::Color::Transparent, float outlineThickness = 1.0f) {
        if (!window_) return;
        sf::RectangleShape r(sf::Vector2f(w, h));
        r.setPosition(x, y);
        r.setFillColor(fill);
        r.setOutlineColor(outline);
        r.setOutlineThickness(outline.a > 0 ? outlineThickness : 0.0f);
        window_->draw(r);
    }

    void bar(float x, float y, float w, float h, float fraction, sf::Color fill,
             sf::Color background = sf::Color(255, 255, 255, 40)) {
        fraction = fraction < 0.0f ? 0.0f : (fraction > 1.0f ? 1.0f : fraction);
        panel(x, y, w, h, background);
        panel(x, y, w * fraction, h, fill);
    }

    void circle(float cx, float cy, float radius, sf::Color fill,
                sf::Color outline = sf::Color::Transparent, float outlineThickness = 1.5f) {
        if (!window_) return;
        sf::CircleShape c(radius, 32);
        c.setOrigin(radius, radius);
        c.setPosition(cx, cy);
        c.setFillColor(fill);
        c.setOutlineColor(outline);
        c.setOutlineThickness(outline.a > 0 ? outlineThickness : 0.0f);
        window_->draw(c);
    }

    // Strzalka (grot + trzonek) o srodku (cx, cy) skierowana pod katem angle w radianach:
    // 0 = w gore ekranu, dodatnio zgodnie z ruchem wskazowek zegara.
    void arrow(float cx, float cy, float size, float angle, sf::Color fill) {
        if (!window_) return;
        sf::Transform transform;
        transform.translate(cx, cy).rotate(angle * 180.0f / 3.14159265f);
        sf::ConvexShape head(3);
        head.setPoint(0, sf::Vector2f(0.0f, -size));
        head.setPoint(1, sf::Vector2f(0.75f * size, 0.0f));
        head.setPoint(2, sf::Vector2f(-0.75f * size, 0.0f));
        head.setFillColor(fill);
        sf::RectangleShape shaft(sf::Vector2f(0.38f * size, 0.95f * size));
        shaft.setPosition(-0.19f * size, -0.05f * size);
        shaft.setFillColor(fill);
        window_->draw(head, transform);
        window_->draw(shaft, transform);
    }

private:
    sf::Font font_;
    bool fontOk_ = false;
    sf::RenderWindow* window_ = nullptr;
};

} // namespace g3d
