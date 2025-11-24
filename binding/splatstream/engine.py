from . import _core


class Engine:
    def __init__(self):
        self.engine = _core.Engine()

    def create_gaussian_splats(self, *args, **kwargs):
        return self.engine.create_gaussian_splats(*args, **kwargs)

    def load_from_ply(self, *args, **kwargs):
        return self.engine.load_from_ply(*args, **kwargs)

    def draw(self, *args, **kwargs):
        return self.engine.draw(*args, **kwargs)

    def show(self, *args, **kwargs):
        return self.engine.show(*args, **kwargs)

    def show_with_cameras(self, *args, **kwargs):
        return self.engine.show_with_cameras(*args, **kwargs)


singleton_engine = Engine()
