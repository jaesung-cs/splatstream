# pygs
Python bindings for 3dgs rendering

## Requirements
- VulkanSDK, with optional components `Volk`, `vma` installed.
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
