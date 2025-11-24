import math
import os

from PIL import Image
import numpy as np
import splatstream as ss

from scene.dataset_readers import readColmapSceneInfo

if __name__ == "__main__":
    splats = ss.load_from_ply("models/train_30000.ply")

    scene = readColmapSceneInfo("models/tandt_db/tandt/train")

    os.makedirs("result", exist_ok=True)
    viewmats = []
    Ks = []
    for i, camera in enumerate(scene.cameras):
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

        viewmats.append(W2C)
        Ks.append(K)

    viewmats = np.stack(viewmats)
    Ks = np.stack(Ks)

    ss.show(splats, viewmats, Ks, width, height)
