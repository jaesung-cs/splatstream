#version 460 core

layout(std430, push_constant) uniform PushConstants {
    mat4 projection;
    mat4 view;
    mat4 model;
};

layout(location = 0) in vec3 position;

layout(location = 0) out vec4 out_position;

void main() {
  gl_Position = projection * view * model * vec4(position, 1.f);
  out_position = view * model * vec4(position, 1.f);
}
