import numpy as np

from . import _core

singleton_module = _core.Module()


def load_from_ply(path: str) -> _core.GaussianSplats:
    return singleton_module.load_from_ply(path)


def draw(
    splats: _core.GaussianSplats,
    view: np.ndarray,
    intrinsic: np.ndarray,
    width: int,
    height: int,
    near: float = 0.01,
    far: float = 100.0,
) -> _core.RenderedImage:
    assert view.shape == (4, 4)
    assert intrinsic.shape == (3, 3)

    view = view.astype(np.float32)
    intrinsic = intrinsic.astype(np.float32)

    view = (
        np.array(
            [[1, 0, 0, 0], [0, -1, 0, 0], [0, 0, -1, 0], [0, 0, 0, 1]], dtype=np.float32
        )
        @ view
    )

    # transform image space
    # (0, 0), (W, H) -> (-1, 1), (1, -1)
    intrinsic = (
        np.array(
            [[2.0 / width, 0, -1], [0, -2.0 / height, 1], [0, 0, 1]], dtype=np.float32
        )
        @ intrinsic
    )

    # np-style intrinsic to vulkan-style projection
    projection = np.zeros((4, 4))
    projection[0, 0] = intrinsic[0, 0]
    projection[1, 1] = intrinsic[1, 1]
    projection[0, 3] = intrinsic[0, 2]
    projection[1, 3] = intrinsic[1, 2]
    projection[2, 2] = far / (near - far)
    projection[2, 3] = near * far / (near - far)
    projection[3, 2] = -1

    return singleton_module.draw(splats, view, projection, width, height)
