import torch

from . import _core
from .module import Module


class Buffer:
    def __init__(self, module: Module, size: int):
        self._buffer = _core.Buffer(module._module, size)
        self._size = size

    @classmethod
    def from_tensor(cls, module: Module, tensor: torch.Tensor):
        if tensor.device == torch.device("cpu"):
            buffer = cls(module, tensor.numel() * tensor.element_size())
            module._module.write_buffer(buffer._buffer, tensor.data_ptr())
            return buffer
        elif tensor.device == torch.device("cuda"):
            raise NotImplementedError("Not implemented for CUDA tensors")
        else:
            raise ValueError(f"Invalid device: {tensor.device}")
