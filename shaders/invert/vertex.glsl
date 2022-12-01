// vertex passthrough shader
in vec2 vert;

out vec2 fragTexCoord;

void main() {
  gl_Position = vec4(vert, 0, 1);
  fragTexCoord = 0.5 * vert + vec2(0.5);
}