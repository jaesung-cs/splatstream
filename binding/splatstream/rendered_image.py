import numpy as np

from . import _core


class RenderedImage:
    def __init__(
        self,
        images: np.ndarray,
        shape: tuple[int],
        rendered_images: list[_core.RenderedImage],
    ):
        self._images = images
        self._shape = shape
        self._rendered_images = rendered_images

    def numpy(self) -> np.ndarray:
        for rendered_image in self._rendered_images:
            rendered_image.wait()
        return self._images.reshape(*self._shape)
