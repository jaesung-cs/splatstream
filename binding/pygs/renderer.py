import numpy as np

from . import _core
from .rendered_image import RenderedImage

singleton_renderer = _core.Renderer()


def load_from_ply(path: str) -> _core.GaussianSplats:
    return singleton_renderer.load_from_ply(path)


def draw(
    splats: _core.GaussianSplats,
    viewmats: np.ndarray,
    Ks: np.ndarray,
    width: int,
    height: int,
    near: float = 0.01,
    far: float = 100.0,
) -> RenderedImage:
    """
    viewmats: (..., 4, 4)
    Ks: (..., 3, 3)
    """
    assert viewmats.shape[-2:] == (4, 4)
    assert Ks.shape[-2:] == (3, 3)
    assert viewmats.shape[:-2] == Ks.shape[:-2]

    batch_dims = viewmats.shape[:-2]

    viewmats = (
        np.array(
            [[1, 0, 0, 0], [0, -1, 0, 0], [0, 0, -1, 0], [0, 0, 0, 1]], dtype=np.float64
        )
        @ viewmats
    )

    # transform image space
    # (0, 0), (W, H) -> (-1, 1), (1, -1)
    Ks = (
        np.array(
            [[2.0 / width, 0, -1], [0, -2.0 / height, 1], [0, 0, 1]], dtype=np.float64
        )
        @ Ks
    )

    # np-style intrinsic to vulkan-style projection
    projections = np.zeros_like(viewmats)
    projections[..., 0, 0] = Ks[..., 0, 0]
    projections[..., 1, 1] = Ks[..., 1, 1]
    projections[..., 0, 3] = Ks[..., 0, 2]
    projections[..., 1, 3] = Ks[..., 1, 2]
    projections[..., 2, 2] = far / (near - far)
    projections[..., 2, 3] = near * far / (near - far)
    projections[..., 3, 2] = -1

    images = np.zeros((*batch_dims, height, width, 4), dtype=np.uint8)

    # flatten
    viewmats = viewmats.reshape(-1, 4, 4)
    projections = projections.reshape(-1, 4, 4)
    images = images.reshape(-1, height, width, 4)

    rendered_images = []
    for i in range(len(images)):
        rendered_images.append(
            singleton_renderer.draw(
                splats, viewmats[i], projections[i], width, height, images[i]
            )
        )

    return RenderedImage(images, (*batch_dims, height, width, 4), rendered_images)
