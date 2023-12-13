#version 330 core

in vec2 tex_coord;
out vec4 col;

uniform sampler2D tex;

void main() {
    col = texture(tex, tex_coord);
}
