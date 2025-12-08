#version 460 core

layout(location = 0) in vec4 position;

layout(location = 0) out vec4 out_depth;  // pre-multiplied alpha: (depth * alpha, alpha)

void main() {
  vec3 p = position.xyz / position.w;
  float d = length(p);
  float alpha = 1.f;  // TODO: alpha from vertex attribute
  out_depth = vec4(d * alpha, alpha, 0.f, alpha);
}
