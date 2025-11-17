# Technical Details

## C++ File Structure
`src/` directory has three components:
- `api`: c++ API wrapper, hiding all Vulkan dependency and exposes the conceptual containers and operations only.
- `core`: main algorithm for rendering Gaussian Splattings, where Vulkan commands for rendering are submitted.
- `viewer`: viewer codes, managing window system and UI, managing GLFW and imgui contexts internally.
- `gpu`: Vulkan backend, with helping features: object lifetime management, object creation, etc.

## Front-to-Back Rendering
The splats are sorted front-to-back, unlike how general renderers draw transparent objects back-to-front.
This is intended in order to calculate the accumulated alpha.

$$
\begin{align*}
C_i &= C_{i-1} + \alpha_i c_i \prod_{j \lt i} (1 - \alpha_j) = C_{i-1} + (\alpha_i c_i) T_{i-1}, \\
T_i &= \prod_{j \le i} (1 - \alpha_j) = (1 - \alpha_i) T_{i-1}.
\end{align*}
$$

The equation turns into the following blending equation:

$$
\begin{align*}
C &:= T_{dst} C_{src} + 1 \cdot C_{dst}, \quad \quad \quad C_{src} = \alpha_{src} c_{src}, \\
T &:= 0 \cdot \alpha_{src} + (1 - \alpha_{src}) \cdot T_{dst}.
\end{align*}
$$

Opacity is one minus transmittance.

$$
\begin{align*}
\Alpha_i &= 1 - T_i = 1 - (1 - \alpha_i) (1 - \Alpha_{i-1}) \\
&= 1 - (1 - \alpha_i) + (1 - \alpha_i) \Alpha_{i-1} \\
&= \alpha_i + (1 - \alpha_i) \Alpha_{i-1}, \\
C &:= (1 - \Alpha_{dst}) \cdot C_{src} + 1 \cdot C_{dst}, \\
\Alpha &:= 1 \cdot \alpha_{src} + (1 - \alpha_{src}) \cdot \Alpha_{dst}
\end{align*}
$$

This leads to the fragment shader pre-multiplied alpha, and Vulkan blend operation, respectively as following:
```glsl
out vec4 out_color;
void main() {
    out_color = vec4(color * alpha, alpha);  // C_{src} \alpha_{src} c_{src}
}
```

```c++
color_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;  // (1 - A_{dst}) . C_{src}
color_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;                  //       1       . C_{dst}
color_attachment.colorBlendOp = VK_BLEND_OP_ADD;
color_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;                  //       1       . a_{src}
color_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;  // (1 - a_{src}) . A_{dst}
color_attachment.alphaBlendOp = VK_BLEND_OP_ADD;
```

Background color shouldn't affect to the alpha, but fill out with the color for the rest of transparency.
These conditions can be written and implemented with the same blending operation.

$$
\begin{align*}
C &:= (1 - \Alpha_{dst}) C_{bg} + 1 \cdot C_{dst}, \\
A &:= 1 \cdot \alpha_{src} + (1 - \alpha_{src}) \cdot \Alpha_{dst} = \Alpha_{dst}, \\
\alpha_{src} &= 0, \quad C_{bg} = c_{bg}.
\end{align*}
$$

```glsl
out vec4 out_color;
void main() {
    out_color = vec4(color_background.rgb, 0.f);
}
```

## Image Format
A pixel color is determined by many small splats, with very small contributions to the pixel.

`R8G8B8A8_UNORM` for color attachment format degrades image quality a lot, because color and alpha values are loaded and stored in blend operations, losing too many significant digits every time.

`R16G16B16A16_SFLOAT` is the best choice; it greatly boosts speed (4x faster than `R32G32B32A32_SFLOAT`) but still retains the image quality (average PSNR is dropped only by 0.02.)

After rendering finishes, `vkCmdBlitImage` command copies to an image of `R8G8B8A8_UNORM` format.
Value clipping between range [0, 1] is automatically performed by blitting.

## Triangle vs. Quad vs. Octagon, Triangle List vs. Strip vs. Fan
The 2d oval splat range can be covered with different geometries: triangle, quad, octagon, any regular polygon.

As the number of vertices increases, the oval area can be bound more tightly, thus reducing number of fragment shader invocation. However, it also increases the length of index buffer, the number of vertex shader invocation, and memory read accesses.

For polygons more complex than polygon, one can still use TRIANGLE_LIST drawing multiple triangles, but TRIANGLE_STRIP and TRIANGLE_FAN are also available and intriguing options.

For example, for octagon covering a circle of radius 1:
- vertices: $r \cdot (\cos(\theta), \sin(\theta))$ where $r = 1 / \cos(\pi / 8)$, $\theta = 2 \pi i / 8$, $i = 0..7$.
- TRIANGLE_LIST: [0, 1, 2, 0, 2, 3, 0, 3, 4, 0, 4, 5, 0, 5, 6, 0, 6, 7], 18 indices.
- TRIANGLE_STRIP: [0, 1, 7, 2, 6, 3, 5, 4, RESTART], 9 indices.
- TRIANGLE_FAN: [0, 1, 2, 3, 4, 5, 6, 7, RESTART], 9 indices.

Note that Metal doesn't support TRIANGLE_FAN but the error message from MoltenVK doesn't say it, reporting undefined behavior instead.

Also, modern hardwares take advantages of vertex caches the most from TRIANGLE_LIST, compared to TRIANGLE_STRIP or TRIANGLE_FAN.

Overall, at this moment, it is found that quad makes the best balance; not too large covered area and the vertex shader invocations, input assembly with TRIANGLE_LIST.

## Double Buffering
Rendering one scene consists of compute, graphics, and transfer stages.
- Compute: sort 3d splats in front-to-back order, and project them into the 2d image space.
- Graphics: open a rasterization render pass to render 2d splats as quads.
- Transfer: copy output image to the output buffer.

GPUs may support dedicated queues for Compute, Graphics, and Transfer tasks depending on the vendor, possibly running asynchronously and thus maximizing throughput.

Let's consider a case of rendering a single scene first.
Say $C_{i}.{stage}$, $G_{i}.{stage}$, and $T_{i}.{stage}$ represent the i-th rendering task in compute, graphics, and transfer queue respectively.
Here's the only synchronization constraint between queues:

$$
\begin{align*}
C_{i}.\text{write} \rightarrow G_{i}.\text{read} \\
G_{i}.\text{write} \rightarrow T_{i}.\text{read}
\end{align*}
$$

Rendering multiple scenes may be fully parallelized because the tasks are independent. However, it requires a lot of memory for intermediate results (projected 2d splats, image, etc.), linearly proportional to the batch size, which could be reused for the next rendering task once previous task is done.

Say we have N-buffer. Then some synchronization constraint are introduced: the intermediate storages in i-th task will be reused in {i+N}-th task.
- Compute write must happens after previous Graphics read.
- Graphics write must happens after previous Transfer read.

$$
\begin{align*}
G_{i}.\text{read} \rightarrow C_{i+N}.\text{write} \\
T_{i}.\text{read} \rightarrow G_{i+N}.\text{write}
\end{align*}
$$

Those 4 synchronizations are sufficient to make sure that all resources are reused with N-buffering.

All together, here shows a diagram demonstrating dependency chains for i, i+N, i+2N, represented with numbers 0, 1, 2 for simplicity (note that i+1, i+2, ..., i+N-1 are still completely independent):
```
+----------------+
| Compute  Queue | [C0.w]      [C1.w]      [C2.w]        ...
+----------------+      \       ^   \       ^   \       ^
                         \C0.0 /G0.0 \C1.0 /G1.0 \C2.0 /G2.0
+----------------+        v   /       v   /       v   /
| Graphics Queue |       [G0.r G0.w] [G1.r G1.w] [G2.r G2.w]        ...
+----------------+                 \       ^   \       ^   \       ^
                                    \G0.1 /T0.0 \G1.1 /T2.0 \G2.1 /T2.1
+----------------+                   v   /       v   /       v   /
| Transfer Queue |                  [T0.r]      [T1.r]      [T2.r]        ...
+----------------+
```
Each queue submission is represented within `[]`. The semaphores are depicted with arrows, representing 3 different timeline semaphores C, G, T and its their stages 0 for C, T and 0, 1 for G.

In a random splat scene, double buffering (N=2) is 10% faster than single buffer (N=1). Triple buffering (N=3) is just the same as double buffering in throughput.

## Dynamic Rendering
Dynamic rendering (`VK_KHR_dynamic_rendering` and `VK_KHR_dynamic_rendering_local_read`) is a recent feature that was promoted to 1.4 and makes the rendering process and management of objects much easier.

Like drawing general scenes where opaque objects are rendered first then transparent objects, we want to draw the opaque scene first to update color and depth buffer, and then Gaussian splattings over the scene. So, the opaque scene is drawn in subpass 0, and gaussian splattings are drawn in subpass 1.

One crucial point is that we need at least float16 image for the image quality of Gaussian splatting. So, rendering Gaussian splattings over the color image of 8-bit precision `B8G8R8A8_UNORM` isn't what we want. Instead, the splats are rendered on a transient image of `R16G16B16A16_SFLOAT` and then blended in subpass 2.
- Attachment 0: swapchain image (`B8G8R8A8_UNORM`)
- Attachment 1: opaque image (`B8G8R8A8_UNORM`, transient)
- Attachment 2: Gaussian splat image (`R16G16B16A16_SFLOAT`, transient)
- Depth attachment (WIP)
- Subpass 0: draw opaque scene to Attachment 1, with depth attachment read/write.
- Subpass 1: draw Gaussian splats to Attachment 2, with depth attachment read (depth test only).
- Subpass 2: blend pixel colors of input Attachment 1 and 2 to Attachment 0.

ImGui's dynamic rendering is partially supported, in that multiple attachments are not available for now.
So, UI is rendered to the swapchain image in a separate render pass.
