#version 460 core

layout(std430, binding = 0) readonly buffer Instances {
  vec4 instances[];  // (N, 12). 3 for ndc position, 1 radius, 4 for rot scale, 4 for color.
};

layout(location = 0) out vec4 out_color;
layout(location = 1) out vec2 out_position;

void main() {
  // index [0,1,2,2,1,3], 4 vertices for a splat.
  int index = gl_VertexIndex / 4;
  vec4 ndc_position = instances[index * 3 + 0];  // xyz, radius
  mat2 rot_scale = mat2(instances[index * 3 + 1].xy, instances[index * 3 + 1].zw);
  vec4 color = instances[index * 3 + 2];

  // quad positions (-1, -1), (-1, 1), (1, -1), (1, 1), ccw in screen space.
  int vert_index = gl_VertexIndex % 4;
  vec2 position = vec2(vert_index / 2, vert_index % 2) * 2.f - 1.f;

  gl_Position = vec4(ndc_position.xyz + vec3(rot_scale * position * ndc_position.w, 0.f), 1.f);
  out_color = color;
  out_position = position * ndc_position.w;
}
