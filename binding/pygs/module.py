from . import _core


class Module:
    def __init__(self):
        self._module = _core.Module()

    @property
    def device_name(self):
        return self._module.device_name

    @property
    def graphics_queue_index(self):
        return self._module.graphics_queue_index

    @property
    def compute_queue_index(self):
        return self._module.compute_queue_index

    @property
    def transfer_queue_index(self):
        return self._module.transfer_queue_index

    def wait_idle(self):
        self._module.wait_idle()
