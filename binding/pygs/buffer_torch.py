import torch

from .buffer import Buffer


def from_tensor(tensor: torch.Tensor):
    if tensor.device == torch.device("cpu"):
        size = tensor.numel() * tensor.element_size()
        buffer = Buffer(size)
        buffer._buffer.to_gpu(tensor.data_ptr(), size)
        return buffer
    elif tensor.device == torch.device("cuda"):
        raise NotImplementedError("Not implemented for CUDA tensors")
    else:
        raise ValueError(f"Invalid device: {tensor.device}")
