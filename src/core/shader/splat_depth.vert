#version 460 core

layout(std430, push_constant) uniform SplatPushConstants {
  mat4 projection_inverse;
  float confidence_radius;
};

#ifdef VKGS_SHADER_INSTANCES_VEC4
layout(std430, binding = 0) readonly buffer Instances {
  vec4 instances[];  // (N, 12). 3 for ndc position, 1 dummy, 4 for rot scale, 4 for color.
};
#else
layout(std430, binding = 0) readonly buffer Instances {
  float instances[];  // (N, 11). 3 for ndc position, 4 for rot scale, 4 for color.
};
#endif

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
#ifdef VKGS_SHADER_INSTANCES_VEC4
  vec3 ndc_position = instances[index * 3 + 0].xyz;
  vec4 rot_scale_vec = instances[index * 3 + 1];
  vec4 color = instances[index * 3 + 2];
  mat2 rot_scale = mat2(rot_scale_vec.xy, rot_scale_vec.zw);
#else
  vec3 ndc_position = vec3(instances[index * 11 + 0], instances[index * 11 + 1], instances[index * 11 + 2]);
  mat2 rot_scale = mat2(instances[index * 11 + 3], instances[index * 11 + 4], instances[index * 11 + 5], instances[index * 11 + 6]);
  vec4 color = vec4(instances[index * 11 + 7], instances[index * 11 + 8], instances[index * 11 + 9], instances[index * 11 + 10]);
#endif

  // circle positions (-1, 0), (0, 1), (1, 0), (0, -1), ccw in screen space.
  vec2 position = positions[gl_VertexIndex % 4];

  float radius = sqrt(max(confidence_radius * confidence_radius + 2.f * log(color.a), 0.f));

  gl_Position = vec4(ndc_position + vec3(rot_scale * position * radius, 0.f), 1.f);
  out_color = color;
  out_position = position * radius;

  out_view_position = projection_inverse * vec4(ndc_position, 1.f);
}
