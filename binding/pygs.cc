#include <pybind11/pybind11.h>

#include "vkgs/buffer.h"
#include "vkgs/module.h"

namespace py = pybind11;

PYBIND11_MODULE(_core, m) {
  py::class_<vkgs::Module>(m, "Module")
      .def(py::init<>())
      .def_property_readonly("device_name", &vkgs::Module::device_name)
      .def_property_readonly("graphics_queue_index", &vkgs::Module::graphics_queue_index)
      .def_property_readonly("compute_queue_index", &vkgs::Module::compute_queue_index)
      .def_property_readonly("transfer_queue_index", &vkgs::Module::transfer_queue_index)
      .def("wait_idle", &vkgs::Module::WaitIdle)
      .def("write_buffer", [](vkgs::Module* module, vkgs::Buffer* buffer, intptr_t ptr) {
        module->WriteBuffer(*buffer, reinterpret_cast<void*>(ptr));
      });
  py::class_<vkgs::Buffer>(m, "Buffer")
      .def(py::init<vkgs::Module&, size_t>())
      .def_property_readonly("size", &vkgs::Buffer::size);
}
