#include <pybind11/pybind11.h>

int add(int i, int j) { return i + j; }

namespace py = pybind11;

PYBIND11_MODULE(pygs_test, m) {
  m.def("add", &add);
  m.def("sub", [](int i, int j) { return i - j; });
}
