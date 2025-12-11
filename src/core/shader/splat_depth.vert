#version 460 core

layout(std430, push_constant) uniform SplatPushConstants {
  mat4 projection_inverse;
  float confidence_radius;
};

struct Row {
  vec4 data[3];  // (N, 12). 3 for ndc position, 1 dummy, 4 for rot scale, 4 for color.
};
layout(std430, binding = 0) readonly buffer Instances {
  Row instances[];  
};

layout(location = 0) out vec4 out_color;
layout(location = 1) out vec2 out_position;
layout(location = 2) out vec4 out_view_position;

const vec2 positions[4] = vec2[4](
  vec2(-1.f, -1.f),
  vec2(-1.f, 1.f),
  vec2(1.f, 1.f),
  vec2(1.f, -1.f)
);

void main() {
  // index [0,1,2,0,2,3], 4 vertices for a splat.
  int index = gl_VertexIndex / 4;
  Row row = instances[index];
  vec3 ndc_position = row.data[0].xyz;
  vec4 rot_scale_vec = row.data[1];
  vec4 color = row.data[2];
  mat2 rot_scale = mat2(rot_scale_vec.xy, rot_scale_vec.zw);

  // circle positions (-1, 0), (0, 1), (1, 0), (0, -1), ccw in screen space.
  vec2 position = positions[gl_VertexIndex % 4];

  float radius = sqrt(max(confidence_radius * confidence_radius + 2.f * log(color.a), 0.f));

  gl_Position = vec4(ndc_position + vec3(rot_scale * position * radius, 0.f), 1.f);
  out_color = color;
  out_position = position * radius;

  out_view_position = projection_inverse * vec4(ndc_position, 1.f);
}
