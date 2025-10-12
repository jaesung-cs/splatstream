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
      .def("draw", [](vkgs::Module& module, vkgs::GaussianSplats splats, py::array_t<float> py_view,
                      py::array_t<float> py_projection, uint32_t width, uint32_t height) {
        const auto* view_ptr = static_cast<const float*>(py_view.request().ptr);
        const auto* projection_ptr = static_cast<const float*>(py_projection.request().ptr);

        // row-major data to mat
        std::vector<float> view;
        view.reserve(16);
        for (int c = 0; c < 4; ++c) {
          for (int r = 0; r < 4; ++r) {
            view.push_back(view_ptr[r * 4 + c]);
          }
        }

        std::vector<float> projection;
        projection.reserve(16);
        for (int c = 0; c < 4; ++c) {
          for (int r = 0; r < 4; ++r) {
            projection.push_back(projection_ptr[r * 4 + c]);
          }
        }

        return module.draw(splats, view.data(), projection.data(), width, height);
      });

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
