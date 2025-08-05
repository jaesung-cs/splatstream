from pybind11.setup_helpers import Pybind11Extension, build_ext
from setuptools import setup

ext_modules = [
  Pybind11Extension(
    "pygs_test",
    ["binding/pygs_test.cc"],
  ),
]

setup(
  name="pygs_test",
  author="Jaesung Park",
  author_email="jaesung.cs@gmail.com",
  url="https://github.com/jaesung-cs/pygs",
  ext_modules=ext_modules,
  cmdclass={"build_ext": build_ext},
  zip_safe=False,
  python_requires=">=3.8",
)
