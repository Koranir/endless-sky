#version 330 core

in vec2 tex_coord;
out vec4 col;

uniform sampler2D tex;
uniform vec4 tex_col;

void main() {
    col = texture(tex, tex_coord);
    col *= tex_col;
}
