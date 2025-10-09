#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>

#include "vkgs/module.h"
#include "vkgs/gaussian_splats.h"
#include "vkgs/rendered_image.h"

namespace py = pybind11;

PYBIND11_MODULE(_core, m) {
  py::class_<vkgs::Module>(m, "Module")
      .def(py::init<>())
      .def_property_readonly("device_name", &vkgs::Module::device_name)
      .def_property_readonly("graphics_queue_index", &vkgs::Module::graphics_queue_index)
      .def_property_readonly("compute_queue_index", &vkgs::Module::compute_queue_index)
      .def_property_readonly("transfer_queue_index", &vkgs::Module::transfer_queue_index)
      .def("load_from_ply", &vkgs::Module::load_from_ply)
      .def("draw", &vkgs::Module::draw);

  py::class_<vkgs::GaussianSplats>(m, "GaussianSplats")
      .def(py::init<>())
      .def_property_readonly("size", &vkgs::GaussianSplats::size);

  py::class_<vkgs::RenderedImage>(m, "RenderedImage")
      .def(py::init<>())
      .def("numpy", [](const vkgs::RenderedImage& rendered_image) {
        return py::array_t<uint8_t>({rendered_image.height(), rendered_image.width(), 4u},
                                    rendered_image.data().data());
      });
}
