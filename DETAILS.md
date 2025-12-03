# Technical Details

## C++ File Structure
`src/` directory has three components:
- `vkgs`: c++ API wrapper, hiding all Vulkan dependency and exposes the conceptual containers and operations only.
- `core`: main algorithm for rendering Gaussian Splattings, where Vulkan commands for rendering are submitted.
- `viewer`: viewer codes, managing window system and UI, managing GLFW and imgui contexts internally.
- `gpu`: Vulkan backend, with helping features: object lifetime management, object creation, etc.

## Vulkan Object Lifetime Management
In `gpu` module, `std::shared_ptr` is meant to manage lifetime of Vulkan resources.
A `std::shared_ptr` as a class member means "as long as this object is alive, objects that it depends must be alive as well."

The seed of all other Vulkan objects is the Vulkan device.
The `Device` class is defined normally, and a singleton Vulkan device is created and retrieved with `GetDevice()` function.
It returns a `std::shared_ptr<Device>` type to be shared by requested objects, and also it is wanted to be destroyed when no objects are used.

The trick is using `std::weak_ptr<Device>` to manage the device like a singleton object.
In `vkgs/gpu/gpu.cc`, `GetDevice()` is defined as:
```cpp
namespace {
std::weak_ptr<Device> device;
}

std::shared_ptr<Device> GetDevice() {
  if (device.expired()) {
    // Not just `return device = std::make_shared<Device>();`.
    // Instead, create a shared ptr, let a weak ptr points to it while keeping the object's lifetime until returning.
    auto new_device = std::make_shared<Device>();
    device = new_device;
    return new_device;
  }
  // If the weak_ptr still points to a valid object, return it.
  return device.lock();
}
```

Another crucial point is that all Vulkan objects needed by GPU commands submitted to queue must be alive until they are finished and signaled by fences.
The objects include `VkPipeline`, `VkPipelineLayout`, `VkBuffer`, `VkImage`, `VkSwapchainKHR`, etc.
To generalize this necessity, wrappers of all Vulkan object types that would be used in commands derive from `Object` class.
Calling `Object::Keep()` while recording a command buffer guarantees that the `std::shared_ptr` of the object sustains until the fence is signaled.
The `std::shared_ptr` of the object is kept in `Task` object.
Tasks are monitored regularly and desteoyed when the attached fences are signaled.

One of the biggest challenges of this design is that the object management system must be considered very carefully not to make dangling `std::shared_ptr` cycles.

Other design patterns for managing lifetime of Vulkan objects could be a single context having `unique_ptr`s of dependent resources, where public interface for the resources are available via handles or wrapper classes.
This design is not adopted because it seemingly makes the design more complex and requires more lines of codes.

## Shared Accessor
`std::shared_ptr` is good for Vulkan resource lifetime management, but is too low-level, in that the users need to create with `std::make_shared` and manage the pointers by their own.

How about having a wrapper class, from which an object is created behaving like `std::shared_ptr` but hold the 

```cpp
template <typename ObjectType, typename InstanceType>
class SharedAccessor {
 public:
  // Empty object
  SharedAccessor() = default;

  // Actual instance is created by Create(...)
  template <typename... Args>
  static ObjectType Create(Args&&... args) {
    auto instance = std::make_shared<InstanceType>(std::forward<Args>(args)...);
    ObjectType object;
    object.instance_ = instance;
    return object;
  }

  // Use as if a variable has functions
  auto operator->() const noexcept { return instance_.get(); }

  // To "any" cast operator, if the implementation class has cast operation.
  template <typename T> operator T() const noexcept { return static_cast<T>(*instance_.get()); }

 private:
  std::shared_ptr<InstanceType> instance_;
};
```

This is the basic pattern:
```cpp
class FooImpl {  // Actual implementation
 public:
  FooImpl(int x, float y);
  void Bar();
};

class Foo : public SharedAccessor<Foo, FooImpl> {};  // Object class

{
  Foo foo;                        // An empty object
  Foo bar = Foo::Create(1, 2.f);  // Create concrete
  bar->Boo();                     // Act like a shared ptr
  Foo baz = bar;                  // An object points to the same one
}                                 // Destructor ~FooImpl() is called once
```

For singleton, `std::weak_ptr<FooImpl>` points to the instance, and `Foo` is created with `Foo::FromPtr` if not expired, `Foo::Create` otherwise.
For more details about this, see `vkgs/gpu/gpu.cc` for more details.

I find this wrapper class very powerful in that the "pointer" concept can be completely hidden in the middle layer and to the end users.

One of drawbacks is that to have shared accessors as member variable, the class is not just to be declared but must be defined, i.e. headers must be included.
It will result in increase of compile time and the number of headers to expose.

Pimpl idiom is also valid along with this shared accessor pattern.
It is good for hiding implementation details and reducing compile time.

## Back-to-Front Rendering
The splats are sorted back-to-front, like how general renderers draw transparent objects.

$$
\begin{align*}
C_i &= \alpha_i c_i + (1 - \alpha_i) C_{i+1} = 1 \cdot (\alpha_i c_i) + (1 - \alpha_i) C_{i+1}, \\
T_i &= \alpha_i + (1 - \alpha_i) T_{i+1} = 1 \cdot \alpha_i + (1 - \alpha_i) T_{i+1}
\end{align*}
$$

This leads to the fragment shader pre-multiplied alpha, and Vulkan blend operation, respectively as following:
```glsl
out vec4 out_color;
void main() {
    out_color = vec4(color * alpha, alpha);  // \alpha_{i} c_{i}
}
```

```c++
color_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;                  //       1       . c_{i}
color_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;  // (1 - a_{src}) . C_{i+1}
color_attachment.colorBlendOp = VK_BLEND_OP_ADD;
color_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;                  //       1       . a_{i}
color_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;  // (1 - a_{src}) . T_{i+1}
color_attachment.alphaBlendOp = VK_BLEND_OP_ADD;
```

Background can be filled with clear color:
```c++
color_attachment.clearValue.color = {background.r, background.g, background.b, 0.f};
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

## Depth Rendering
The four points of quads in Normalized Device Coordinates (NDC) are available in vertex shaders.
The homogeneous coordinates multiplied by inverse of projection matrix yield and linearly interpolated by rasterizer yields a homogeneous coordinate in viewer space in fragment shader.
The distance from camera to fragment is just the length of the vector.

Blend the depth values to get the weighted depth value as well as color value:
$$
\begin{align*}
D_i = \alpha_i d_i + (1 - \alpha_i) D_{i+1},
\end{align*}
$$
to an `R16G16_SFLOAT` image (or `R16_SFLOAT` because alpha values are also available in color image.)

### Outlining
Drawing borders is simpliy achieved by rendering dark if the gradient of depth is higher than threshold.

```glsl
float depth = subpassInput(depth_iamge).r;
float gradient = fwidth(depth);  // fwidth is the same as abs(dFdx(depth)) + abs(dFdy(depth)).
```

One of disadvantages is that `fwidth`, `dFdx`, `dFdy` is just a finite difference; GPU groups four quad pixels to calculate those values, making the gradient values in any 2-pixel square the same.
This makes borders very coarse and thick.

A good property of Gaussian Splats for this purpose is that the splat is differentiable.
The analytic difference can be driven by hand, but I leave it as TODO because it is not available for triangular meshes that I wanted to draw in viewer.

## Dynamic Rendering
Dynamic rendering (`VK_KHR_dynamic_rendering` and `VK_KHR_dynamic_rendering_local_read`) is a recent feature that was promoted to 1.4 and makes the rendering process and management of objects much easier.

Like drawing general scenes where opaque objects are rendered first then transparent objects, we want to draw the opaque scene first to update color and depth buffer, and then Gaussian splattings over the scene. So, the opaque scene is drawn in subpass 0, and gaussian splattings are drawn in subpass 1.

One crucial point is that we need at least float16 image for the image quality of Gaussian splatting. So, rendering Gaussian splattings over the color image of 8-bit precision `B8G8R8A8_UNORM` isn't what we want. Instead, the splats are rendered on a transient image of `R16G16B16A16_SFLOAT` and then blended in subpass 2.
- Attachment 0: swapchain image (`B8G8R8A8_UNORM`)
- Attachment 1: high-res image (`R16G16B16A16_SFLOAT`, transient)
- Attachment 2: depth image (`R16G16_SFLOAT`, transient)
- Depth attachment
- Subpass 0:
  - draw opaque scene to Attachment 1, with depth attachment read/write.
  - draw Gaussian splats to Attachment 1 and 2, with depth attachment read (depth test only).
- Subpass 1:
  - blend pixel colors of input Attachment 1 and 2 to Attachment 0.

ImGui's dynamic rendering is partially supported, in that multiple attachments are not available for now.
So, UI is rendered to the swapchain image in a separate render pass.

### Memory Access Pattern
When it comes to calculation and memory costs, Gaussian splatting is leaned toward memory-bound operation, i.e. the number of operations are not so much compared to the number of memory reads/writes.

Considering memory coalescing is as import as the global memory size and read/write count.

It turns out that writing to screen splat instances into (N, 12) tensor, 12 elements of each row aggregated with 3 `vec4`s, is faster by 10% FPS in "garden" scene than into (N, 11) tensor with 11 `float`s.
Even the memory consumption is approximately 10% more, the end-to-end frame rate is faster by 10% (FPS 400 vs. 360)
This is probably because of cache flush cost on write operations. The former requires 3 writes, whiel the latter requires 11 writes.

Because writing operation is more vulnerable to random access pattern, I also tested the "sequential gaussian splat read + random screen splat write" vs. "random gaussian splat read + sequential screen splat write."

SH coefficients are (N, 48) `float16` tensor which is equivalent to 24 `float`s, and is still large.
Other tensors are position (N, 3), cov3d (N, 6), and opacity (N, 1).

With the current sequential read + random write scheme, FPS was 400.
With random read + sequential write, FPS was dropped significantly to 300.
This can be simply tested by using inversed index in `projection.comp`.
Having On/Off flag for this makes code too complicated to just test the rendering speed.

My conclusion is that sequential read + random write is the best so far, but there is still more room for improvement.
Current implementation has many inactivate invocations for splats that are not visible.

Shared memory could be a key to improve the rendering speed further.
