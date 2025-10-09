from . import _core

singleton_module = _core.Module()


def load_from_ply(path: str) -> _core.GaussianSplats:
    return singleton_module.load_from_ply(path)


def draw(splats: _core.GaussianSplats) -> _core.RenderedImage:
    return singleton_module.draw(splats)
