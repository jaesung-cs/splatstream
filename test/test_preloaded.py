from PIL import Image
import math
import os

from plyfile import PlyData
import numpy as np
import splatstream as ss

from scene.dataset_readers import readColmapSceneInfo


def sigmoid(x):
    return 1.0 / (1.0 + np.exp(-x))


def load_ply(ply_path):
    ply = PlyData.read(ply_path)
    vertex = ply["vertex"].data
    names = vertex.dtype.names

    if "x" in names and "f_rest_44" in names:
        print("[INFO] Detected float format")

        def extract_stack(fields):
            return np.stack([vertex[f] for f in fields], axis=1)

        vertex_data = {
            "position": extract_stack(["x", "y", "z"]),
            "f_dc": extract_stack(["f_dc_0", "f_dc_1", "f_dc_2"]),
            "f_rest": extract_stack([f"f_rest_{i}" for i in range(45)]),
            "opacity": sigmoid(np.array(vertex["opacity"])),
            "scale": np.exp(extract_stack(["scale_0", "scale_1", "scale_2"])),
            "rotation": extract_stack(["rot_0", "rot_1", "rot_2", "rot_3"]),
        }
        return vertex_data

    else:
        raise ValueError("Unsupported .ply format. Use uncompressed ply file.")


if __name__ == "__main__":
    ply_path = "../pygs/models/train_30000.ply"
    data = load_ply(ply_path)
    gcolors = np.concatenate(
        (data["f_dc"][:, None, :], data["f_rest"].reshape(-1, 3, 15).swapaxes(-1, -2)),
        axis=-2,
    )

    print("loading splats...")
    splats = ss.gaussian_splats(
        means=data["position"],
        quats=data["rotation"],
        scales=data["scale"],
        colors=gcolors,
        opacity=data["opacity"],
    )
    print("loading splats done")

    scene = readColmapSceneInfo("../pygs/models/tandt_db/tandt/train")

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

    N = 10
    viewmats = viewmats[:N]
    Ks = Ks[:N]

    print("draw start")
    images = ss.draw(
        splats=splats,
        viewmats=viewmats,
        Ks=Ks,
        width=width,
        height=height,
        sh_degree=3,
        near=0.1,
        far=1e3,
    ).numpy()
    print("draw end")

    colors = [Image.fromarray(images[i, :, :, :3]) for i in range(len(images))]
    alphas = [Image.fromarray(images[i, :, :, 3]) for i in range(len(images))]

    os.makedirs("result", exist_ok=True)
    # for i in range(len(colors)):
    #    colors[i].save(f"result/{i+1:05d}.png")
    #    alphas[i].save(f"result/{i+1:05d}_alpha.png")
    gif_path = f"result/result.gif"
    colors[0].save(
        gif_path, save_all=True, append_images=colors[1:], duration=100, loop=0
    )
