import numpy as np

from . import _core


class RenderingTask:
    def __init__(
        self,
        images: np.ndarray,
        shape: tuple[int],
        task: _core.RenderingTask,
    ):
        self._images = images
        self._shape = shape
        self._task = task

    def numpy(self) -> np.ndarray:
        self._task.wait()
        return self._images.reshape(*self._shape)
