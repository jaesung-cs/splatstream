from .module import Module

singleton_module = Module()


def wait_idle():
    singleton_module._module.wait_idle()
