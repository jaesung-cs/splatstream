from PIL import Image
import time
import math
import os

import numpy as np
import splatstream as ss


def sigmoid(x):
    return 1.0 / (1.0 + np.exp(-x))


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
    N = 100000
    splats = ss.gaussian_splats(
        means=np.random.randn(N, 3) * 30.0,
        quats=_random_quat(N),
        scales=np.random.rand(N, 3) * 0.24 + 0.01,
        opacities=sigmoid(np.random.rand(N) - 2),
        colors=np.random.rand(N, 16, 3) * 2 - 1,
    )

    ss.show(splats)

    width = 128
    height = 128
    fov_x = math.radians(120.0)
    fov_y = math.radians(120.0)

    K = np.zeros((3, 3))
    K[0, 0] = width / (2 * math.tan(fov_x / 2))
    K[1, 1] = height / (2 * math.tan(fov_y / 2))
    K[0, 2] = width / 2
    K[1, 2] = height / 2
    K[2, 2] = 1

    N = 32
    viewmats = []
    backgrounds = []
    for i in range(N):
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

        s = 0.5 * math.sin(theta) + 0.5
        c = 0.5 * math.cos(theta) + 0.5
        background = np.array([0, s, c])

        viewmats.append(W2C)
        backgrounds.append(background)

    viewmats = np.stack(viewmats)
    backgrounds = np.stack(backgrounds)

    print("draw start")
    start_time = time.time()
    image = ss.draw(
        splats, viewmats, K, width, height, far=1e5, backgrounds=backgrounds
    ).numpy()
    end_time = time.time()
    rendering_time = end_time - start_time
    print("draw end")
    print(f"FPS: {N / rendering_time:.2f}")

    image = np.concatenate((image[..., :3], image[..., 3:].repeat(3, axis=-1)), axis=-2)

    for i in range(len(image)):
        s = (i * 2 * width) // N
        e = ((i + 1) * 2 * width) // N
        w = 2
        image[i, (height - height // N) - w :, s - w : e + w, :] = 0
        image[i, (height - height // N) :, s:e, :] = 255

    print("save start")
    os.makedirs("test_random", exist_ok=True)
    imgs = []
    for i in range(len(image)):
        img = Image.fromarray(image[i])
        img.save(f"test_random/{i+1:05d}.png")
        imgs.append(img)

    gif_path = "test_random/result.gif"
    imgs[0].save(gif_path, save_all=True, append_images=imgs[1:], duration=20, loop=0)
    print(f"saved animation to {gif_path}")
