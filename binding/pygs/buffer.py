import torch

from . import _core
from .singleton_module import singleton_module


class Buffer:
    def __init__(self, size: int):
        self._buffer = _core.Buffer(singleton_module._module, size)
        self._size = size

    @property
    def size(self):
        return self._buffer.size


def from_tensor(tensor: torch.Tensor):
    if tensor.device == torch.device("cpu"):
        buffer = Buffer(tensor.numel() * tensor.element_size())
        singleton_module._module.write_buffer(buffer._buffer, tensor.data_ptr())
        return buffer
    elif tensor.device == torch.device("cuda"):
        raise NotImplementedError("Not implemented for CUDA tensors")
    else:
        raise ValueError(f"Invalid device: {tensor.device}")
