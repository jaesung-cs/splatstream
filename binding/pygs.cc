#include <pybind11/pybind11.h>

#include "vkgs/module.h"

namespace py = pybind11;

PYBIND11_MODULE(_core, m) { py::class_<vkgs::Module>(m, "Module").def(py::init<>()); }
