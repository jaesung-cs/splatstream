# Benchmark

## Requirements
- Install `splatstream`
  - From local build:
    ```bash
    $ pip install ./binding
    ```
  - Or from public package:
    ```bash
    $ pip install splatstream
    ```
- Install `torch>=2.6`, with cuda version of your choice.
- Then install `gsplat` and other dependencies
  ```bash
  $ pip install -r ./bench/requirements.txt
  ```

## Usage
```bash
$ python bench/bench.py
```

## Examples
```bash
$ python .\bench\bench.py --ply_path models/train_30000.ply --colmap_path models/tandt_db/tandt/train --scale 0.5 --target splatstream --first 20
...
PSNR: 23.21 ± 1.35
FPS: 107.06

$ python .\bench\bench.py --ply_path models/train_30000.ply --colmap_path models/tandt_db/tandt/train --scale 0.5 --target gsplat --first 20 --chunk_size 10
...
PSNR: 22.50 ± 1.73
FPS: 380.52
```

## Results
- Test environment:
  - NVIDIA GeForce RTX 5080
  - CPU memory 17GB (total 32GB, used 15GB constantly on background)
  - Windows
  - CUDA 13.0
  - torch 2.9.0+cu130
- Notes:
  - chunk_size = 30. When chunk size is over some threshold like 30~60, rendering time greatly increases rendering time for 
    Instead of rendering hundreds of images, the set is splitted by chunks and draw one by one.
    There's no options but to split them by chunks.
  - FPS includes rendering time plus data transfers of image from GPU to CPU.
    For gsplat, data transfer happens every chunk.
    Be careful when interpreting FPS!
  - scale = 0.5, to match the image size with ground truth.
  - near = 0.1.
  - far = 1000. Too high value reduces the depth precisions near camera.

|    | Dataset | scale | #imgs | chunk size | PSNR | FPS |
|:--:|:-------:|:-----:|:-----:|:----------:|:----:|:---:|
| **gsplat**      | truck | 0.5 | 251 |  10 | 20.59 ± 1.66 | **330.61** |
|   splatstream   | truck | 0.5 | 251 | N/A | 20.69 ± 1.58 |   113.78   |
| **gsplat**      | train | 0.5 | 301 |  10 | 22.29 ± 2.41 | **339.76** |
|   splatstream   | train | 0.5 | 301 | N/A | 22.11 ± 2.29 |   111.70   |
