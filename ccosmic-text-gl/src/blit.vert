#version 330 core

in vec2 vert;

out vec2 tex_coord;

uniform vec2 source_pos;
uniform vec2 source_size;

uniform vec2 dest_pos;
uniform vec2 dest_size;

void main() {
    tex_coord = vert * source_size + source_pos;

    gl_Position = vec4((vert * dest_size + dest_pos) * 2 - 1, 0.0, 1.0);
}
