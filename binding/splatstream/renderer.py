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
    near: float | np.ndarray = 0.01,
    far: float | np.ndarray = 100.0,
    backgrounds: np.ndarray | None = None,
    eps2d: float | np.ndarray = 0.3,
) -> RenderedImage:
    """
    viewmats: (..., 4, 4)
    Ks: (..., 3, 3)
    near: (...) or scalar
    far: (...) or scalar
    backgrounds: (..., 3)
    """
    if isinstance(near, (int, float)):
        near = np.array(near)

    if isinstance(far, (int, float)):
        far = np.array(far)

    if isinstance(eps2d, (int, float)):
        eps2d = np.array(eps2d)

    if backgrounds is None:
        backgrounds = np.array([0, 0, 0])

    assert viewmats.shape[-2:] == (4, 4)
    assert Ks.shape[-2:] == (3, 3)
    assert backgrounds.shape[-1:] == (3,)

    # np-style view matrix (Y-down) to vulkan-style (Y-up)
    viewmats = (
        np.array([[1, 0, 0, 0], [0, -1, 0, 0], [0, 0, -1, 0], [0, 0, 0, 1]]) @ viewmats
    )

    # transform image space
    # (0, 0), (W, H) -> (-1, 1), (1, -1)
    Ks = np.array([[2.0 / width, 0, -1], [0, -2.0 / height, 1], [0, 0, 1]]) @ Ks

    # broadcast to batch dims
    batch_dims = np.broadcast_shapes(
        viewmats.shape[:-2],
        Ks.shape[:-2],
        near.shape,
        far.shape,
        backgrounds.shape[:-1],
    )
    viewmats = np.broadcast_to(viewmats, (*batch_dims, 4, 4))
    Ks = np.broadcast_to(Ks, (*batch_dims, 3, 3))
    backgrounds = np.broadcast_to(backgrounds, (*batch_dims, 3))
    eps2d = np.broadcast_to(eps2d, batch_dims)

    # allocate image
    images = np.zeros((*batch_dims, height, width, 4), dtype=np.uint8)

    # np-style intrinsic to vulkan-style projection
    projections = np.insert(Ks, 2, 0, axis=-1)
    projections = np.insert(projections, 2, 0, axis=-2)
    projections[..., 2, 2] = far / (near - far)
    projections[..., 2, 3] = near * far / (near - far)
    projections[..., 3, 2] = -1

    # flatten
    viewmats = np.ascontiguousarray(viewmats.reshape(-1, 4, 4))
    projections = np.ascontiguousarray(projections.reshape(-1, 4, 4))
    backgrounds = np.ascontiguousarray(backgrounds.reshape(-1, 3))
    eps2d = np.ascontiguousarray(eps2d)
    images = np.ascontiguousarray(images.reshape(-1, height, width, 4))

    rendered_images = []
    for i in range(len(images)):
        rendered_images.append(
            singleton_renderer.draw(
                splats,
                viewmats[i],
                projections[i],
                width,
                height,
                backgrounds[i],
                eps2d[i],
                images[i],
            )
        )

    return RenderedImage(images, (*batch_dims, height, width, 4), rendered_images)
