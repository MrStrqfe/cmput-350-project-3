#version 120
// Stub: draws the sprite unchanged (texture x tint).
uniform sampler2D texture;

void main() {
    gl_FragColor = texture2D(texture, gl_TexCoord[0].xy) * gl_Color;
}