import math

import numpy as np
from plyfile import PlyData

from scene.dataset_readers import readColmapSceneInfo


def sigmoid(x):
    return 1.0 / (1.0 + np.exp(-x))


def load_ply(ply_path):
    ply = PlyData.read(ply_path)
    vertex = ply["vertex"].data

    def extract_stack(fields):
        return np.stack([vertex[f] for f in fields], axis=1)

    means = extract_stack(["x", "y", "z"]).astype(np.float32)
    quats = extract_stack(["rot_0", "rot_1", "rot_2", "rot_3"]).astype(np.float32)
    scales = np.exp(
        extract_stack(["scale_0", "scale_1", "scale_2"]).astype(np.float32)
    )  # exp activation
    opacities = sigmoid(
        np.array(vertex["opacity"]).astype(np.float32)
    )  # sigmoid activation
    f_dc = extract_stack(["f_dc_0", "f_dc_1", "f_dc_2"])
    f_rest = extract_stack([f"f_rest_{i}" for i in range(45)])
    colors = np.concatenate(
        (f_dc[:, None, :], f_rest.reshape(-1, 3, 15).swapaxes(-1, -2)), axis=-2
    ).astype(np.float32)

    vertex_data = {
        "means": means,
        "quats": quats,
        "scales": scales,
        "opacities": opacities,
        "colors": colors,
    }
    return vertex_data


def load_colmap_data(colmap_path: str, scale: float = 1.0, first: int | None = None):
    scene = readColmapSceneInfo(colmap_path)

    assert all(c.width == scene.cameras[0].width for c in scene.cameras)
    assert all(c.height == scene.cameras[0].height for c in scene.cameras)

    width = int(math.ceil(scene.cameras[0].width * scale))
    height = int(math.ceil(scene.cameras[0].height * scale))

    viewmats = []
    Ks = []
    image_paths = []
    for camera in scene.cameras:
        fov_y = camera.FovY
        fov_x = camera.FovX
        R = camera.R
        T = camera.T

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
        image_paths.append(camera.image_path)

    viewmats = np.stack(viewmats)
    Ks = np.stack(Ks)

    # Take first N images
    if first is not None:
        first = min(first, len(viewmats))
        viewmats = viewmats[:first]
        Ks = Ks[:first]
        image_paths = image_paths[:first]

    return {
        "viewmats": viewmats,
        "Ks": Ks,
        "width": width,
        "height": height,
        "image_paths": image_paths,
    }
