#version 120
// ZADANIE 20 -- fragment shader przygotowany do uzycia z wave.vert.
uniform sampler2D uTexture;
varying float vDiffuse;

void main() {
    vec4 texel = texture2D(uTexture, gl_TexCoord[0].st) * gl_Color;
    gl_FragColor = vec4(texel.rgb * clamp(vDiffuse, 0.0, 1.0), texel.a);
}
