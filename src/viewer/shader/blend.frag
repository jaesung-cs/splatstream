#version 460 core

layout (input_attachment_index = 1, binding = 0) uniform subpassInput image_scene;
layout (input_attachment_index = 2, binding = 1) uniform subpassInput image_splat;

layout (location = 0) out vec4 out_color;

void main() {
    vec4 scene = subpassLoad(image_scene);
    vec4 splat = subpassLoad(image_splat);
    // overlay splat on scene. splat is pre-multiplied alpha.
    out_color = vec4(scene.rgb * (1.f - splat.a) + splat.rgb, 1.f);
}
