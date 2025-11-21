#version 460 core

layout (std430, push_constant) uniform PushConstants {
    layout (offset = 128) int mode;
};

layout (input_attachment_index = 0, binding = 0) uniform subpassInput image;

layout (location = 0) out vec4 out_color;

void main() {
    vec4 color = subpassLoad(image);

    if (mode == 0) {  // color
        out_color = color;
    } else if (mode == 1) {  // alpha
        out_color = vec4(color.a);
    }
}
