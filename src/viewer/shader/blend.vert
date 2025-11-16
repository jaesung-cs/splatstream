#version 460 core

void main() {
  const vec4 positions[] = {
    vec4(-1.f, -1.f, 1.f, 1.f),
    vec4(-1.f, 3.f, 1.f, 1.f),
    vec4(3.f, -1.f, 1.f, 1.f),
  };

  gl_Position = positions[gl_VertexIndex % 3];
}
