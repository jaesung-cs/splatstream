from PIL import Image
import math
import os

import numpy as np
import pygs


def _random_quat(N: int):
    u1 = np.random.rand(N)
    u2 = np.random.rand(N)
    u3 = np.random.rand(N)

    q = np.stack(
        (
            np.sqrt(1 - u1) * np.sin(2 * np.pi * u2),
            np.sqrt(1 - u1) * np.cos(2 * np.pi * u2),
            np.sqrt(u1) * np.sin(2 * np.pi * u3),
            np.sqrt(u1) * np.cos(2 * np.pi * u3),
        ),
        axis=-1,
    )
    return q


if __name__ == "__main__":
    os.makedirs("test_random", exist_ok=True)

    splat_path = "test_random/gsplat.ply"

    N = 1000
    with open(splat_path, "wb") as f:
        f.write(b"ply\n")
        f.write(b"format binary_little_endian 1.0\n")
        f.write(f"element vertex {N}\n".encode())
        f.write(b"property float x\n")
        f.write(b"property float y\n")
        f.write(b"property float z\n")
        f.write(b"property float scale_0\n")
        f.write(b"property float scale_1\n")
        f.write(b"property float scale_2\n")
        f.write(b"property float rot_0\n")
        f.write(b"property float rot_1\n")
        f.write(b"property float rot_2\n")
        f.write(b"property float rot_3\n")
        f.write(b"property float f_dc_0\n")
        f.write(b"property float f_dc_1\n")
        f.write(b"property float f_dc_2\n")
        for i in range(45):
            f.write(f"property float f_rest_{i}\n".encode())
        f.write(b"property float opacity\n")
        f.write(b"end_header\n")

        pos = np.random.randn(N, 3).astype(np.float32) * 2.5
        scale = np.log(np.random.rand(N, 3) * 0.49 + 0.01).astype(np.float32)  # exp
        rot = _random_quat(N).astype(np.float32)
        dc = np.random.rand(N, 3).astype(np.float32) * 2 - 1
        rest = np.random.rand(N, 45).astype(np.float32) * 1 - 0.5
        opacity = np.random.rand(N, 1).astype(np.float32) * 3 - 3  # sigmoid

        data = np.ascontiguousarray(
            np.concatenate((pos, scale, rot, dc, rest, opacity), axis=-1)
        )
        f.write(data.tobytes())

    splats = pygs.load_from_ply(splat_path)

    N = 64
    viewmats = []
    Ks = []
    for i in range(N):
        width = 256
        height = 256
        fov_x = math.radians(120.0)
        fov_y = math.radians(120.0)

        # view
        theta = 2.0 * math.pi * (i / N)
        radius = 10.0
        C2W = np.zeros((4, 4))
        C2W[:3, 0] = np.array([-math.sin(theta), 0.0, -math.cos(theta)])
        C2W[:3, 1] = -np.array([0.0, 1.0, 0.0])
        C2W[:3, 2] = -np.array([math.cos(theta), 0.0, -math.sin(theta)])
        C2W[:3, 3] = np.array([math.cos(theta), 0.0, -math.sin(theta)]) * radius
        C2W[3, 3] = 1.0
        W2C = np.linalg.inv(C2W)

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

    print("draw start")
    image = pygs.draw(splats, viewmats, Ks, width, height, far=1e5).numpy()
    print("draw end")

    image = image[..., :3]
    for i in range(len(image)):
        s = (i * height) // N
        e = ((i + 1) * height) // N
        image[i, (width - width // N) :, s:e, :] = 255

    print("save start")
    imgs = []
    for i in range(len(image)):
        img = Image.fromarray(image[i])
        img.save(f"test_random/{i+1:05d}.png")
        imgs.append(img)

    gif_path = "test_random/result.gif"
    imgs[0].save(gif_path, save_all=True, append_images=imgs[1:], duration=20, loop=0)
    print(f"saved animation to {gif_path}")
