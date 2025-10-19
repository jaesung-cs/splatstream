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

        # Resize
        width = width // 2
        height = height // 2

        # view
        W2C = np.zeros((4, 4))
        W2C[:3, :3] = R.transpose()
        W2C[:3, 3] = T
        W2C[3, 3] = 1

        # intrinsic
        K = np.zeros((3, 3))
        K[0, 0] = width / (2 * math.tan(fov_x / 2))
        K[1, 1] = height / (2 * math.tan(fov_y / 2))
        K[0, 2] = width / 2
        K[1, 2] = height / 2
        K[2, 2] = 1

        image = pygs.draw(splats, W2C, K, width, height, near=1.0, far=1e3).numpy()

        print(camera.image_path)
        color = Image.fromarray(image[..., :3])
        alpha = Image.fromarray(image[..., 3])
        color.save(f"result/{i+1:05d}.png")
        alpha.save(f"result/{i+1:05d}_alpha.png")
