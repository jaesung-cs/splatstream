from . import _core


class Engine:
    def __init__(self):
        _core.init()
        self.engine = _core.Engine()

    def __del__(self):
        del self.engine
        _core.terminate()
        print("terminate complete")

    def create_gaussian_splats(self, *args, **kwargs):
        return self.engine.create_gaussian_splats(*args, **kwargs)

    def load_from_ply(self, *args, **kwargs):
        return self.engine.load_from_ply(*args, **kwargs)

    def draw(self, *args, **kwargs):
        return self.engine.draw(*args, **kwargs)


singleton_engine = Engine()
