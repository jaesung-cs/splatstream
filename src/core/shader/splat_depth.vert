#version 460 core

layout(std430, push_constant) uniform SplatPushConstants {
  mat4 projection_inverse;
  float confidence_radius;
};

layout(std430, binding = 0) readonly buffer Instances {
  vec4 instances[];    // (N, 12). 3 for ndc position, 1 alpha, 4 for rot scale, 3 for color.
};

layout(location = 0) out float out_alpha;
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
  // TODO: stride 3 -> 2
  vec4 pa = instances[3 * index + 0];
  vec4 rot_scale_vec = instances[3 * index + 1];
  vec3 ndc_position = pa.xyz;
  float alpha = pa.a;
  mat2 rot_scale = mat2(rot_scale_vec.xy, rot_scale_vec.zw);

  // circle positions (-1, 0), (0, 1), (1, 0), (0, -1), ccw in screen space.
  vec2 position = positions[gl_VertexIndex % 4];

  float radius = sqrt(max(confidence_radius * confidence_radius + 2.f * log(alpha), 0.f));

  gl_Position = vec4(ndc_position + vec3(rot_scale * position * radius, 0.f), 1.f);
  out_alpha = alpha;
  out_position = position * radius;

  out_view_position = projection_inverse * vec4(ndc_position, 1.f);
}
