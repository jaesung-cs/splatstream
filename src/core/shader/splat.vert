#version 460 core

layout(std430, binding = 0) readonly buffer Instances {
  vec4 instances[];  // (N, 12). 3 for ndc position, 1 radius, 4 for rot scale, 4 for color.
};

layout(location = 0) out vec4 out_color;
layout(location = 1) out vec2 out_position;

const vec2 positions[4] = vec2[4](
  vec2(-1.f, -1.f),
  vec2(-1.f, 1.f),
  vec2(1.f, 1.f),
  vec2(1.f, -1.f)
);

void main() {
  // index [0,1,2,0,2,3], 4 vertices for a splat.
  int index = gl_VertexIndex / 4;
  vec4 ndc_position = instances[index * 3 + 0];  // xyz, radius
  vec4 rot_scale_vec = instances[index * 3 + 1];
  vec4 color = instances[index * 3 + 2];

  mat2 rot_scale = mat2(rot_scale_vec.xy, rot_scale_vec.zw);

  // circle positions (-1, 0), (0, 1), (1, 0), (0, -1), ccw in screen space.
  vec2 position = positions[gl_VertexIndex % 4];

  gl_Position = vec4(ndc_position.xyz + vec3(rot_scale * position * ndc_position.w, 0.f), 1.f);
  out_color = color;
  out_position = position * ndc_position.w;
}
