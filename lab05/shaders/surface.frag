#version 120
// ZADANIE 19 -- desaturacja (odcienie szarosci) i pulsowanie jasnosci.
uniform sampler2D uTexture;   // aktualnie zwiazana tekstura (sf::Shader::CurrentTexture, jednostka 0)
uniform float uGrayscale;     // 0.0 = oryginalny kolor, 1.0 = pelna szarosc
uniform float uPulse;         // 0.0 = stala jasnosc,   1.0 = pulsowanie
uniform float uTime;          // czas animacji w sekundach (stoi podczas pauzy)

void main() {
    vec4 texel = texture2D(uTexture, gl_TexCoord[0].st) * gl_Color;

    // Jasnosc wzgledna (wagi ITU-R BT.601) i mieszanie koloru z szaroscia.
    float luminance = dot(texel.rgb, vec3(0.299, 0.587, 0.114));
    vec3 color = mix(texel.rgb, vec3(luminance), uGrayscale);

    // Wspolczynnik jasnosci: 1.0 albo 0.80 + 0.20 * sin(2.5 * t).
    float brightness = mix(1.0, 0.80 + 0.20 * sin(2.5 * uTime), uPulse);

    gl_FragColor = vec4(color * brightness, texel.a);   // alfa bez zmian
}
