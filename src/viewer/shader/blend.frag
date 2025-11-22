#version 460 core

layout (std430, push_constant) uniform PushConstants {
    layout (offset = 128) int mode;
};

layout (input_attachment_index = 0, binding = 0) uniform subpassInput color_image;
layout (input_attachment_index = 1, binding = 1) uniform subpassInput depth_image;

layout (location = 0) out vec4 out_color;

void main() {
    vec4 color = subpassLoad(color_image);
    vec4 depth = subpassLoad(depth_image);

    if (mode == 0) {  // color
        out_color = color;
    } else if (mode == 1) {  // alpha
        out_color = vec4(color.a);
    } else if (mode == 2) {  // depth
        float d = depth.r / depth.g;
        const float gap = 1.f;
        d = depth.g > 0.f ? mod(d / gap, 1.f) : 0.f;
        d = d > 0.5f ? 1.f - d : d;
        const float threshold = 0.05f;
        d = d < threshold ? 1.f - (d - threshold) : 0.f;
        d = 1.f - d;

        vec3 display_color = vec3(d * color.rgb);
        out_color = vec4(display_color * depth.g, depth.g);
    }
}
