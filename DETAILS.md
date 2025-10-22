# Technical Details

## C++ File Structure
`src/` directory has three components:
- `api`: c++ API wrapper, hiding all Vulkan dependency and exposes the conceptual containers and operations only.
- `core`: main algorithm for rendering Gaussian Splattings, where Vulkan commands for rendering are submitted.
- `gpu`: Vulkan backend, with helping features: object lifetime management, object creation, etc.

## Front-to-Back Rendering
The splats are sorted front-to-back, unlike how general renderers draw transparent objects back-to-front.
This is intended in order to calculate the accumulated alpha.

$$
\begin{align*}
C_i &= C_{i-1} + \alpha_i c_i \prod_{j \lt i} (1 - \alpha_j) = C_{i-1} + (\alpha_i c_i) T_{i-1} \\
T_i &= \prod_{j \le i} (1 - \alpha_j) = (1 - \alpha_i) T_{i-1}
\end{align*}
$$

The equation turns into fragment

$$
\begin{align*}
C &:= \alpha_{dst} C_{src} + 1 \cdot C_{dst}, \quad \quad \quad C_{src} = \alpha_{src} c_{src} \\
\alpha &:= 0 \cdot \alpha_{src} + (1 - \alpha_{src}) \cdot \alpha_{dst}
\end{align*}
$$

This leads to the fragment shader pre-multiplied alpha, and Vulkan blend operation as following:
```c++
color_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;
color_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
color_attachment.colorBlendOp = VK_BLEND_OP_ADD;
color_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
color_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
color_attachment.alphaBlendOp = VK_BLEND_OP_ADD;
```

## Image Format
A pixel color is determined by many small splats, with very small contributions to the pixel.
The image must be in format of `R32G32B32A32_SFLOAT` to blend correctly, where color and alpha values are loaded and stored in every blend operation.
Then, `vkCmdBlitImage` command draws it to another image of `R8G8B8A8_UNORM` format.
Value clipping between range [0, 1] is automatically performed by blitting.

## Double Buffering
...
