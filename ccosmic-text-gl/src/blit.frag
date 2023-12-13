#version 330 core

in vec2 tex_coord;
out vec4 col;

uniform sampler2D tex;

void main() {
    col = texture(tex, tex_coord);
    col += texture(tex, vec2(0.5, 0.5)).r == 0.0 ? 1. : 0.;
}
