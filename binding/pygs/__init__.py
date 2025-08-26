from .module import Module

# Global module on init
singleton_module = Module()

from .buffer import Buffer, from_tensor

__all__ = [
    "Buffer",
    "from_tensor",
]
