from ._core import add, sub


def wrapped_add(i, j):
    return add(i, j)


def wrapped_sub(i, j):
    return sub(i, j)
