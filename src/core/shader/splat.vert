#version 460 core

layout(std430, binding = 0) readonly buffer Instances {
  vec4 instances[];  // (N, 12). 3 for ndc position, 1 padding, 4 for rot scale, 4 for color.
};

layout(location = 0) out vec4 out_color;
layout(location = 1) out vec2 out_position;

void main() {
  // index [0,1,2,3,4,5,6,7,8,1-1], 9 vertices, 11 indices for a splat.
  int index = gl_VertexIndex / 9;
  vec3 ndc_position = instances[index * 3 + 0].xyz;
  mat2 rot_scale = mat2(instances[index * 3 + 1].xy, instances[index * 3 + 1].zw);
  vec4 color = instances[index * 3 + 2];

  // vertexpositions (-cos(theta), sin(theta)), ccw in screen space.
  int vert_index = gl_VertexIndex % 9;
  vec2 position;
  if (vert_index == 0) {
    position = vec2(0.f, 0.f);
  } else {
    const float PI = 3.14159265358979323846;
    const float alpha = 2.f * PI / 8.f;
    float theta = alpha * (vert_index - 1);
    position = vec2(-cos(theta), sin(theta)) / cos(alpha / 2.f);
  }

  float confidence_radius = 3.33f;

  gl_Position = vec4(ndc_position + vec3(rot_scale * position * confidence_radius, 0.f), 1.f);
  out_color = color;
  out_position = position * confidence_radius;
}
