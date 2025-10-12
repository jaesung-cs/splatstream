from PIL import Image
import math
import os

import numpy as np
import pygs

from scene.dataset_readers import readColmapSceneInfo

if __name__ == "__main__":
    splats = pygs.load_from_ply("models/train_30000.ply")

    scene = readColmapSceneInfo("models/tandt_db/tandt/train")

    os.makedirs("result", exist_ok=True)
    for i, camera in enumerate(scene.cameras):
        print(i)

        width = camera.width
        height = camera.height
        fov_y = camera.FovY
        fov_x = camera.FovX
        R = camera.R
        T = camera.T

        # view
        C2W = np.zeros((4, 4))
        C2W[:3, :3] = R.transpose()
        C2W[:3, 3] = T
        C2W[3, 3] = 1
        # W2C = np.linalg.inv(C2W)
        W2C = C2W

        # intrinsic
        K = np.zeros((3, 3))
        K[0, 0] = width / (2 * math.tan(fov_x / 2))
        K[1, 1] = height / (2 * math.tan(fov_y / 2))
        K[0, 2] = width / 2
        K[1, 2] = height / 2
        K[2, 2] = 1

        image = pygs.draw(splats, W2C, K, width, height, far=1e6).numpy()

        print(camera.image_path)
        image = Image.fromarray(image, "RGBA")
        image.save(f"result/{i+1:05d}.png")
