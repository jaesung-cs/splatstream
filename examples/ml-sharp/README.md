# ML-SHARP: Sharp Monocular View Synthesis in Less Than a Second

[[Project Page]](https://gynjn.github.io/MVP/)
[[Github Repo]](https://github.com/Gynjn/MVP)

This directory shows an example of visualizing Multi-view Pyramid Transformer pre-trained models.

See their LICENSE for the license information.

## Install

- Install `torch` with CUDA 12.8 and `sharp`
    ```bash
    $ conda create -n sharp python=3.13
    $ conda activate sharp
    $ (isntall torch)
    $ pip install git+https://github.com/apple/ml-sharp@1eaa046834b81852261262b41b0919f5c1efdd2e
    ```

- Install `splatstream`
    ```bash
    $ pip install splatstream
    or
    $ pip install ./binding --no-build-isolation
    ```

- Download model (follow the guideline in the repo)

- Run `visualize.py`
    ```bash
    $ python visualize.py --input_path <image_path>
    ```
    image path to PNG file, etc.

- Check "Graphics > Gamma Correction" button to visualize color in sRGB color space.
