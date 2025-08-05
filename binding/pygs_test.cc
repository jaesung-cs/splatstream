#include <pybind11/pybind11.h>

#include "vkgs/vkgs.h"

namespace py = pybind11;

PYBIND11_MODULE(pygs_test, m) {
  m.def("add", &vkgs::add);
  m.def("sub", [](int i, int j) { return i - j; });
}
