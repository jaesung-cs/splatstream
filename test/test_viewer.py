import argparse
import json

import numpy as np
import splatstream as ss

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--scene", type=str, default="bonsai")
    args = parser.parse_args()

    scene = args.scene

    splats = ss.load_from_ply(f"models/{scene}/point_cloud.ply")
    with open(f"models/{scene}/cameras.json", "r") as f:
        cameras = json.load(f)

    viewmats = np.stack([camera["extrinsic"] for camera in cameras])
    Ks = np.stack([camera["intrinsic"] for camera in cameras])
    width = cameras[0]["width"]
    height = cameras[0]["height"]

    ss.show(splats, viewmats, Ks, width, height)
