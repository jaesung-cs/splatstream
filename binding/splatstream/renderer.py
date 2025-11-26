import numpy as np

from . import _core
from .rendered_image import RenderedImage
from .engine import singleton_engine


def gaussian_splats(
    means: np.ndarray,
    quats: np.ndarray,
    scales: np.ndarray,
    opacities: np.ndarray,
    colors: np.ndarray,
) -> _core.GaussianSplats:
    """
    means: (N, 3)
    quats: (N, 4), wxyz convention.
    scales: (N, 3)
    opacities: (N)
    colors: (N, 3) for sh degree = 0, or (N, K, 3) sh coefficients. K = 1, 4, 9, or 16.
    """
    if colors.ndim == 2:
        colors = colors[:, None, :]

    assert means.ndim == 2 and means.shape[-1] == 3
    assert quats.ndim == 2 and quats.shape[-1] == 4
    assert scales.ndim == 2 and scales.shape[-1] == 3
    assert colors.ndim == 3 and colors.shape[-1] == 3
    assert opacities.ndim == 1
    assert (
        means.shape[0]
        == quats.shape[0]
        == scales.shape[0]
        == colors.shape[0]
        == opacities.shape[0]
    )

    K = colors.shape[-2]
    assert K in [1, 4, 9, 16]

    sh_degree = {1: 0, 4: 1, 9: 2, 16: 3}[K]

    quats = quats / np.linalg.norm(quats, axis=-1, keepdims=True)

    means = np.ascontiguousarray(means, dtype=np.float32)
    quats = np.ascontiguousarray(quats, dtype=np.float32)
    scales = np.ascontiguousarray(scales, dtype=np.float32)
    colors = np.ascontiguousarray(colors, dtype=np.float16)
    opacities = np.ascontiguousarray(opacities, dtype=np.float32)

    # float16 is not acceptable by pybind11, so pass the pointer instead.
    colors_ptr = colors.ctypes.data

    return singleton_engine.create_gaussian_splats(
        means, quats, scales, opacities, colors_ptr, sh_degree
    )


def load_from_ply(path: str, sh_degree: int = -1) -> _core.GaussianSplats:
    return singleton_engine.load_from_ply(path, sh_degree)


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
    sh_degree: int | np.ndarray = -1,
) -> RenderedImage:
    """
    viewmats: (..., 4, 4)
    Ks: (..., 3, 3)
    near: (...) or scalar
    far: (...) or scalar
    backgrounds: (..., 3)
    eps2d: (...) or scalar
    sh_degree: (...) or scalar. -1 for max degree.
    """
    if isinstance(near, (int, float)):
        near = np.array(near)

    if isinstance(far, (int, float)):
        far = np.array(far)

    if isinstance(eps2d, (int, float)):
        eps2d = np.array(eps2d)

    if isinstance(sh_degree, int):
        sh_degree = np.array(sh_degree)

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
        eps2d.shape,
        sh_degree.shape,
    )
    viewmats = np.broadcast_to(viewmats, (*batch_dims, 4, 4))
    Ks = np.broadcast_to(Ks, (*batch_dims, 3, 3))
    backgrounds = np.broadcast_to(backgrounds, (*batch_dims, 3))
    eps2d = np.broadcast_to(eps2d, batch_dims)
    sh_degree = np.broadcast_to(sh_degree, batch_dims)

    # allocate image
    images = np.zeros((*batch_dims, height, width, 4), dtype=np.uint8)

    # np-style intrinsic to vulkan-style projection
    projections = np.insert(Ks, 2, 0, axis=-1)
    projections = np.insert(projections, 2, 0, axis=-2)
    projections[..., 2, 2] = far / (near - far)
    projections[..., 2, 3] = near * far / (near - far)
    projections[..., 3, 2] = -1
    projections[..., 3, 3] = 0

    # flatten
    viewmats = np.ascontiguousarray(viewmats.reshape(-1, 4, 4))
    projections = np.ascontiguousarray(projections.reshape(-1, 4, 4))
    backgrounds = np.ascontiguousarray(backgrounds.reshape(-1, 3))
    eps2d = np.ascontiguousarray(eps2d.reshape(-1))
    sh_degree = np.ascontiguousarray(sh_degree.reshape(-1))
    images = np.ascontiguousarray(images.reshape(-1, height, width, 4))

    rendered_images = []
    for i in range(len(images)):
        rendered_images.append(
            singleton_engine.draw(
                splats,
                viewmats[i],
                projections[i],
                width,
                height,
                backgrounds[i],
                eps2d[i],
                sh_degree[i],
                images[i],
            )
        )

    return RenderedImage(images, (*batch_dims, height, width, 4), rendered_images)


def show(
    splats: _core.GaussianSplats,
    viewmats: np.ndarray | None = None,
    Ks: np.ndarray | None = None,
    width: int | None = None,
    height: int | None = None,
):
    """
    viewmats: (..., 4, 4)
    Ks: (..., 3, 3)
    width: width of camera param
    height: height of camera param
    """
    if (
        viewmats is not None
        and Ks is not None
        and width is not None
        and height is not None
    ):
        batch_dims = np.broadcast_shapes(
            viewmats.shape[:-2],
            Ks.shape[:-2],
        )
        viewmats = np.broadcast_to(viewmats, (*batch_dims, 4, 4))
        Ks = np.broadcast_to(Ks, (*batch_dims, 3, 3))

        viewmats = np.ascontiguousarray(viewmats.reshape(-1, 4, 4), dtype=np.float32)
        Ks = np.ascontiguousarray(Ks.reshape(-1, 3, 3), dtype=np.float32)
        singleton_engine.show_with_cameras(splats, viewmats, Ks, width, height)
    else:
        singleton_engine.show(splats)
