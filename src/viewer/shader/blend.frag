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
        const float max_depth = 10.f;
        float d = 1.f - clamp((depth.r / depth.g) / max_depth, 0.f, 1.f);
        float gradient = fwidth(d);
        const float edge_threshold = 0.01f;
        const float edge_width = 0.01f;
        float border = smoothstep(edge_threshold, edge_threshold + edge_width, gradient / (1.f - d));

        vec3 display_color = vec3(0.f) * border + d * (1.f - border);
        out_color = vec4(display_color * depth.g, depth.g);
    }
}
