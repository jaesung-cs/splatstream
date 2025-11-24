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
        self._done = False
        self.compute_timestamps = np.zeros(len(tasks), dtype=np.uint64)
        self.graphics_timestamps = np.zeros(len(tasks), dtype=np.uint64)
        self.transfer_timestamps = np.zeros(len(tasks), dtype=np.uint64)

    def __del__(self):
        self.wait()

    def numpy(self) -> np.ndarray:
        self.wait()
        return self._images.reshape(*self._shape)

    def wait(self):
        if self._done:
            return

        for i, task in enumerate(self._tasks):
            task.wait()
            draw_result = task.draw_result()
            self.compute_timestamps[i] = draw_result.compute_timestamp
            self.graphics_timestamps[i] = draw_result.graphics_timestamp
            self.transfer_timestamps[i] = draw_result.transfer_timestamp
        self._done = True
