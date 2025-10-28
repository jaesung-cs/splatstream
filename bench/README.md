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
PSNR: 23.20 ± 1.35
FPS: 438.01

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
- Dataset
  | Dataset | #imgs | scale | resolution | #points |
  |:--------|:-----:|:-----:|:----------:|:-------:|
  | bicycle | 194   |     4 |   1237x822 | 6131954 |
  | bonsai  | 292   |     2 |  1559x1039 | 1244819 |
  | counter | 240   |     2 |  1558x1038 | 1222956 |
  | garden  | 185   |     4 |   1297x840 | 5834784 |
  | kitchen | 279   |     2 |  1558x1039 | 1852335 |
  | room    | 311   |     2 |  1557x1038 | 1593376 |
  | truck   | 251   |     1 |    979x546 | 2541226 |
  | train   | 301   |     1 |    980x545 | 1026508 |
- Benchmakr Result
  | Implementation | Dataset | chunk size | PSNR | FPS |
  |:---------------|:--------|:----------:|:----:|:---:|
  | gsplat      | bicycle |  2  |   19.18 ± 1.60   |    245.70   |
  | splatstream | bicycle | N/A | **19.41 ± 1.67** |  **256.59** |
  | gsplat      | bonsai  |  2  |   30.91 ± 2.35   |  **380.56** |
  | splatstream | bonsai  | N/A | **31.12 ± 2.23** |    281.56   |
  | gsplat      | counter |  2  |   29.35 ± 1.40   |    295.98   |
  | splatstream | counter | N/A | **29.44 ± 1.41** |  **318.02** |
  | gsplat      | garden  |  2  |   18.96 ± 0.74   |    196.80   |
  | splatstream | garden  | N/A | **19.30 ± 0.74** |  **221.60** |
  | gsplat      | kitchen |  2  |   29.69 ± 1.88   |    186.26   |
  | splatstream | kitchen | N/A | **29.97 ± 1.83** |  **242.94** |
  | gsplat      | room    |  2  | **32.37 ± 1.88** |  **339.28** |
  | splatstream | room    | N/A |   32.11 ± 1.78   |    319.37   |
  | gsplat      | truck   |  2  |   20.59 ± 1.66   |    392.87   |
  | splatstream | truck   | N/A | **20.67 ± 1.58** |  **415.46** |
  | gsplat      | train   |  2  | **22.29 ± 2.41** |    431.35   |
  | splatstream | train   | N/A |   22.11 ± 2.29   |  **511.62** |
