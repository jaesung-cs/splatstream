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
      .def("wait_idle", &vkgs::Module::WaitIdle);
  py::class_<vkgs::Buffer>(m, "Buffer")
      .def(py::init<vkgs::Module&, size_t>())
      .def_property_readonly("size", &vkgs::Buffer::size)
      .def("to_gpu",
           [](vkgs::Buffer* buffer, intptr_t ptr, size_t size) { buffer->ToGpu(reinterpret_cast<void*>(ptr), size); })
      .def("to_cpu",
           [](vkgs::Buffer* buffer, intptr_t ptr, size_t size) { buffer->ToCpu(reinterpret_cast<void*>(ptr), size); });
}
