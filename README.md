# SplatStream

[![Build Status](https://github.com/jaesung-cs/splatstream/workflows/Build%20Python%20Package/badge.svg)](https://github.com/jaesung-cs/splatstream/actions/workflows/build.yml)
[![Release Status](https://github.com/jaesung-cs/splatstream/workflows/Release%20Python%20Package/badge.svg)](https://github.com/jaesung-cs/splatstream/actions/workflows/release.yml)
[![Python Versions](https://img.shields.io/badge/python-3.10%20%7C%203.11%20%7C%203.12%20%7C%203.13%20%7C%203.14-blue.svg)](https://www.python.org/downloads/)
[![Platforms](https://img.shields.io/badge/platform-linux%20%7C%20windows%20%7C%20macos-lightgrey.svg)](https://github.com/jaesung-cs/splatstream/actions)

Python bindings for streamlined 3D Gaussian Splatting rendering.
- This is a more modularized and organized code of my previous c++ project [vkgs](https://github.com/jaesung-cs/vkgs).

https://github.com/user-attachments/assets/7ee79ceb-0c8d-491c-8517-b07ba0ac7fbe

Splatstream interactive viewer for "garden" scene.

https://github.com/user-attachments/assets/1750e48a-d9ef-4a1c-bdb7-2b1877e70ecf

Splatstream interactive viewer for [MVP](https://gynjn.github.io/MVP/) pre-trained model.

See [`examples/MVP`](examples/MVP) for more details.

## Feature Highlights
- Fast rendering: GPU-accelerated rasterization for high performance
  - Up to 2x faster than [vk_gaussian_splatting](https://github.com/nvpro-samples/vk_gaussian_splatting) viewer
  - Up to 1.3x faster than [gsplat](https://github.com/nerfstudio-project/gsplat) off-screen rendering
- Memory-efficient: resource reuse through double buffering
- Pure Vulkan implementation with minimal dependencies, no CUDA required
- Compatible with pre-trained models from the [original Gaussian Splatting](https://github.com/graphdeco-inria/gaussian-splatting)
- Interactive viewer

## Installation
Install the stable version from PyPI:
```bash
$ pip install splatstream
```

Or install the development version from source:
```bash
$ pip install --no-build-isolation ./binding
```

## Usage
```python
import splatstream as ss

# Splats from .ply file
splats = ss.load_from_ply("my_splats.ply")

# Or, from numpy array
splats = ss.gaussian_splats(
    means,      # np.ndarray, (N, 3)
    quats,      # np.ndarray, (N, 4), wxyz convention.
    scales,     # np.ndarray, (N, 3)
    opacities,  # np.ndarray, (N)
    colors,     # np.ndarray, (N, K, 3), where K is one of [1, 4, 9, 16]
)

# Show in viewer
ss.show(splats)  # opens an interactive viewer

# Show in viewer with camera params
ss.show(
    splats,
    viewmats,          # np.ndarray, (..., 4, 4)
    Ks,                # np.ndarray, (..., 3, 3)
    width,             # int
    height,            # int
)

# Draw to numpy array
images = ss.draw(
    splats,
    viewmats,          # np.ndarray, (..., 4, 4)
    Ks,                # np.ndarray, (..., 3, 3)
    width,             # int
    height,            # int
    near=0.1, far=1e3  # Do not use too low near nor too high far, otherwise z-fighting
).numpy()              # np.ndarray, (..., H, W, 4), np.uint8, RGBA.

from PIL import Image
for i in range(len(images)):
  Image.fromarray(images[i, :, :, :3]).save(f"color_{i}.png")
  Image.fromarray(images[i, :, :, 3]).save(f"alpha_{i}.png")
```

### Viewer Camera Controls
- WASD/Space to move
- Q/E to roll
- Left drag to rotate
- Right drag to move
- L+R drag or wheel to zoom
- Ctrl wheel to dolly zoom (change FOV angle)

## Requirements
- GPU with Vulkan 1.4+ support ([check compatibility](https://vulkan.gpuinfo.org/listdevices.php))
- Up-to-date graphics drivers
- Python >= 3.10
- Supported platforms: Windows x64, Ubuntu x64, macOS arm64
- Monitor required for the viewer program

## Limitations
- No gradient computation; rendering only, not training
- Perfect pinhole cameras only
- Rendered images may differ slightly from [gsplat](https://github.com/nerfstudio-project/gsplat) (likely due to splat culling logic)
- No PyTorch integration

## Pre-trained Dataset
This project redistributes a modified subset of the [Gaussian Splatting](https://github.com/graphdeco-inria/gaussian-splatting) dataset.

The dataset is available [here](https://drive.google.com/drive/folders/1bfmbi84C4Xy51fVSdXLqKpb3y8Pt7Gia?usp=sharing) (Google Drive link).

See the `LICENSE` file in the dataset for details.

Download the datasets to the `models/` directory.

To run the viewer with a pre-trained 3DGS scene:
```bash
$ python test/test_viewer.py --scene <scene>
$ python test/test_viewer.py --scene <scene> --no-camera
```

where `<scene>` is one of: `bicycle`, `bonsai`, `counter`, etc.

## Benchmark
Tested on NVIDIA GeForce RTX 5080, Windows. FPS measure includes **rendering time plus GPU-to-CPU transfer**.

| Implementation | Dataset | #imgs | resolution | #splats | PSNR | FPS |
|:---------------|:--------|:-----:|:----------:|:-------:|:----:|:---:|
| gsplat      | bicycle | 194 |  1237x822 | 6131954 |   19.18 ± 1.60   |    245.70   |
| splatstream | bicycle | 194 |  1237x822 | 6131954 | **19.22 ± 1.67** |  **373.08** |
| gsplat      | garden  | 185 |  1297x840 | 5834784 |   18.96 ± 0.74   |    196.80   |
| splatstream | garden  | 185 |  1297x840 | 5834784 | **19.11 ± 0.74** |  **320.93** |

See [bench](/bench/README.md) for more details.

## Technical Details
See [DETAILS.md](/DETAILS.md).

## Development Requirements
- `VulkanSDK>=1.4.328.1` (set up environment variables per OS)
- `python>=3.10`
- `cmake>=3.15` (install via `pip install cmake` or `conda install conda-forge::cmake`)
- python packages (install via `pip install`)
    - `scikit-build-core`

## Contributing
Feedback, feature requests, and issue reports are welcome!

## TODO
- [ ] Device selection
