import argparse

from PIL import Image
import numpy as np

from common import load_ply, load_colmap_data


def calculate_psnr(img1: np.ndarray, img2: np.ndarray):
    mse = np.mean((img1.astype(np.float32) - img2.astype(np.float32)) ** 2)
    if mse == 0:
        return 100
    return 20 * np.log10(255.0 / np.sqrt(mse))


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--ply_path", type=str, required=True)
    parser.add_argument("--colmap_path", type=str, required=True)
    parser.add_argument(
        "--scale",
        type=int,
        default=1,
        help="Scale of the image, one of [1, 2, 4, 8].",
    )
    parser.add_argument(
        "--target",
        type=str,
        choices=["gsplat", "splatstream"],
        default="splatstream",
        help="Benchmark target rendering implementation.",
    )
    parser.add_argument(
        "--first", type=int, help="First N images to benchmark (debugging)"
    )
    parser.add_argument(
        "--chunk_size",
        type=int,
        default=2,
        help="Chunk size for rendering (gsplat only)",
    )
    args = parser.parse_args()

    ply_path = args.ply_path
    colmap_path = args.colmap_path
    scale = args.scale
    target = args.target
    N = args.first
    chunk_size = args.chunk_size

    # Load data
    print("loading ply data...")
    ply_data = load_ply(ply_path)
    print("loading colmap data...")
    draw_data = load_colmap_data(colmap_path, scale=scale, first=N)
    print("loading data done")

    # Benchmark
    if target == "splatstream":
        from draw_splatstream import draw_splatstream

        result = draw_splatstream(ply_data, draw_data)
    elif target == "gsplat":
        from draw_gsplat import draw_gsplat

        result = draw_gsplat(ply_data, draw_data, chunk_size=chunk_size)
    else:
        raise ValueError(f"Invalid target: {target}")

    # Evaluate
    psnrs = []
    for i in range(len(result["colors"])):
        img0 = np.array(Image.open(draw_data["image_paths"][i]))
        img1 = result["colors"][i]
        psnr = calculate_psnr(img0, img1)
        psnrs.append(psnr)
    psnr_mean = np.mean(psnrs)
    psnr_std = np.std(psnrs)
    print(f"#points: {len(ply_data['means'])}")
    print(f"resolution: {draw_data['width']}x{draw_data['height']}")
    print(f"#imgs: {len(result['colors'])}")
    print(f"chunk: {chunk_size}")
    print(f"PSNR: {psnr_mean:.2f} ± {psnr_std:.2f}")
    print(f"FPS: {len(result['colors']) / result['total_time']:.2f}")
