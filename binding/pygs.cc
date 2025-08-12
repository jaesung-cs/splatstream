#include <pybind11/pybind11.h>

#include "vkgs/vkgs.h"

namespace py = pybind11;

PYBIND11_MODULE(_core, m) {
  m.def("add", &vkgs::add, "Add two integers");
  m.def("sub", [](int i, int j) { return i - j; }, "Subtract two integers");
}
