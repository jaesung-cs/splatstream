#version 460 core

layout (input_attachment_index = 0, binding = 0) uniform subpassInput depth_image;

layout (location = 0) out vec4 out_color;

void main() {
  vec4 depth = subpassLoad(depth_image);

  const float max_depth = 10.f;
  float d = 1.f - clamp((depth.r / depth.g) / max_depth, 0.f, 1.f);
  // TODO: border
  /*
  float gradient = fwidth(d);
  const float edge_threshold = 0.01f;
  const float edge_width = 0.01f;
  float border = smoothstep(edge_threshold, edge_threshold + edge_width, gradient / (1.f - d));
  */
  float border = 0.f;

  vec3 display_color = vec3(0.f) * border + d * (1.f - border);
  out_color = vec4(display_color * depth.g, depth.g);
}
