#version 460 core

layout (std430, push_constant) uniform PushConstants {
    mat4 projection;
    mat4 view;
};

layout (location = 0) out vec4 color;

const vec4 positions[6] = vec4[6](
    vec4(0.f, 0.f, 0.f, 1.f),
    vec4(1.f, 0.f, 0.f, 1.f),
    vec4(0.f, 1.f, 0.f, 1.f),
    vec4(0.f, 1.f, 0.f, 1.f),
    vec4(1.f, 0.f, 0.f, 1.f),
    vec4(1.f, 1.f, 0.f, 1.f)
);

void main() {
    vec4 position = positions[gl_VertexIndex % 6];
    gl_Position = projection * view * position;
    color = position;
}
