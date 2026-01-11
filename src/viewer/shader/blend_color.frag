#version 460 core

layout (std430, push_constant) uniform PushConstants {
  int mode;
  int gamma_correction;
};

layout (input_attachment_index = 0, binding = 0) uniform subpassInput color_image;

layout (location = 0) out vec4 out_color;

vec3 rgb_to_srgb(vec3 rgb) {
  return 1.055 * pow(rgb, vec3(1 / 2.4)) - 0.055;
}

void main() {
  vec4 color = subpassLoad(color_image);

  if (mode == 0) {  // color
    if (gamma_correction == 1) {
      out_color = vec4(rgb_to_srgb(color.rgb), color.a);
    } else {
      out_color = color;
    }
  } else if (mode == 1) {  // alpha
    out_color = vec4(color.a);
  }
}
