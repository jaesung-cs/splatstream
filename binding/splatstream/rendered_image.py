import numpy as np

from . import _core


class RenderedImage:
    def __init__(
        self,
        images: np.ndarray,
        shape: tuple[int],
        tasks: list[_core.RenderingTask],
    ):
        self._images = images
        self._shape = shape
        self._tasks = tasks

    def numpy(self) -> np.ndarray:
        for task in self._tasks:
            task.wait()
        return self._images.reshape(*self._shape)
