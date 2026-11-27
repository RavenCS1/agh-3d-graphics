#version 120
// ZADANIE 20 -- deformacja fali, normalna i oswietlenie rozproszone w shaderze wierzcholkow.
uniform float uPhase;       // faza = 1.5 * czas animacji (ta sama wartosc co na CPU)
uniform float uAmplitude;   // amplituda fali [0, 0.6]
uniform float uGpuWave;     // 1.0 = wysokosc liczona tutaj, 0.0 = gotowe dane z CPU
uniform float uFrequency;   // rozszerzenie: mnoznik czestotliwosci (1.0 = wzor z zadania 15)
uniform vec3 uLightWorld;   // polozenie swiatla w ukladzie swiata
varying float vDiffuse;
const float PI = 3.14159265358979323846;
const float BASE_Y = -0.35;

void main() {
    vec4 p = gl_Vertex;
    vec3 normal = gl_Normal;

    if (uGpuWave > 0.5) {
        // h(x, z) = -0.35 + A * sin(k*x - phase) * cos(k*z + phase),  k = PI * czestotliwosc
        float k = PI * uFrequency;
        float sx = sin(k * p.x - uPhase);
        float cx = cos(k * p.x - uPhase);
        float sz = sin(k * p.z + uPhase);
        float cz = cos(k * p.z + uPhase);
        p.y = BASE_Y + uAmplitude * sx * cz;

        // Normalna powierzchni y = h(x, z): (-dh/dx, 1, -dh/dz)
        float dhdx =  uAmplitude * k * cx * cz;
        float dhdz = -uAmplitude * k * sx * sz;
        normal = vec3(-dhdx, 1.0, -dhdz);
    }

    // Oswietlenie Lamberta w przestrzeni kamery. Siatka jest rysowana bez wlasnej
    // transformacji modelu, wiec gl_ModelViewMatrix jest tu macierza widoku i mozna
    // nia przeksztalcic takze polozenie swiatla podane w ukladzie swiata.
    vec3 positionEye = vec3(gl_ModelViewMatrix * p);
    vec3 N = normalize(gl_NormalMatrix * normal);
    vec3 lightEye = vec3(gl_ModelViewMatrix * vec4(uLightWorld, 1.0));
    vec3 L = normalize(lightEye - positionEye);
    vDiffuse = 0.25 + 0.75 * max(dot(N, L), 0.0);

    gl_Position = gl_ModelViewProjectionMatrix * p;
    gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;
    gl_FrontColor = gl_Color;
}
