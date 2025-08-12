from skbuild import setup

setup(
    name="pygs",
    author="Jaesung Park",
    author_email="jaesung.cs@gmail.com",
    url="https://github.com/jaesung-cs/pygs",
    python_requires=">=3.8",
    cmake_source_dir=".",
    packages=["pygs"],
    package_dir={"pygs": "binding/pygs"},
)
