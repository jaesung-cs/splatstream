#version 460 core

layout(push_constant, std430) uniform PushConstants {
  vec4 background_color;
};

layout (location = 0) out vec4 out_color;

void main() {
  out_color = vec4(background_color.rgb, 0.f);
}
