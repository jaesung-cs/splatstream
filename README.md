# pygs
Python bindings for 3dgs rendering

## Requirements
- VulkanSDK, with optional components `Volk`, `vma` installed.
  - `MacOS`: `>=1.4.328.1`, becuase `VK_KHR_push_descriptor` is supported by `MoltenVK` since then.
  - others: `>=1.4`
- cmake
- Dependencies
```bash
$ pip install -r requirements.txt
```

## Build c++
```bash
$ cmake . -B build
$ cmake --build build --config Release -j
```

## Build Python Module
```bash
$ pip install ./binding
```
