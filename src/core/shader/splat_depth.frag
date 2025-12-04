#version 460 core

layout(location = 0) in vec4 color;
layout(location = 1) in vec2 position;
layout(location = 2) in vec4 view_position;

layout(location = 0) out vec4 out_depth;  // pre-multiplied alpha: (depth * alpha, alpha)

void main() {
  float gaussian_alpha = exp(-0.5f * dot(position, position));
  float alpha = color.a * gaussian_alpha;

  vec3 p = view_position.xyz / view_position.w;
  float d = length(p);
  out_depth = vec4(d * alpha, alpha, 0.f, alpha);
}
