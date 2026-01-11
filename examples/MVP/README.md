# Multi-view Pyramid Transformer

[[Project Page]](https://gynjn.github.io/MVP/)
[[Github Repo]](https://github.com/Gynjn/MVP)

This directory shows an example of visualizing Multi-view Pyramid Transformer pre-trained models.

The original codes under MIT license are copy-and-pasted and modified to visualize inference results with splatstream.

## Install

1. Install requirements
    ```bash
    # create conda environment
    conda create -n mvp python=3.11 -y
    conda activate mvp

    # install PyTorch (adjust cuda version according to your system)
    pip install -r requirements.txt

    # install splatstream
    pip install splatstream
    ```

1. Download the pre-trained model in the repo

1. Follow the guideline to prepare for downloading dataset (huggingface)

1. Run `download_and_undistort.py`
    ```bash
    python download_and_undistort.py --subset hash --hash 9641a1ed7963ce5ca734cff3e6ccea3dfa8bcb0b0a3ff78f65d32a080de2d71e`
    ```

1. Prepare `data/dl3dv_visualize.txt` and `data/dl3dv_fold_8_kmeans_input_idx_visualize.json`
    - Modify the contents accordingly, from the MVP github repo.

## Run

```bash
python visualize.py --config configs/visualize.yaml
```
