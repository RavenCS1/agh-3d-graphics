#version 120
// ZADANIE 19 -- gotowy shader wierzcholkow (geometria bez zmian).
// Macierz tekstury SFML moze odwrocic os V; uwzgledniaj ja jawnie.
void main() {
    gl_Position = ftransform();
    gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;
    gl_FrontColor = gl_Color;
}
