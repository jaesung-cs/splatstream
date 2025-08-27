from typing import Any

import numpy as np

from . import _core
from .singleton_module import singleton_module


class Buffer:
    def __init__(self, size: int):
        self._buffer = _core.Buffer(singleton_module._module, size)
        self._size = size

    @property
    def size(self):
        return self._buffer.size

    def to_numpy(self, dtype: Any = np.uint8):
        dtype = np.dtype(dtype)
        buf = np.empty((self.size // dtype.itemsize), dtype=dtype)
        self._buffer.to_cpu(buf.ctypes.data, self.size)
        return buf

    def fill(self, value: int):
        self._buffer.fill(value)
        return self


def from_numpy(arr: np.ndarray):
    size = arr.size * arr.itemsize
    buffer = Buffer(size)
    buffer._buffer.to_gpu(arr.ctypes.data, size)
    return buffer
