#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>

#include "vkgs/renderer.h"
#include "vkgs/gaussian_splats.h"
#include "vkgs/rendered_image.h"

namespace py = pybind11;

PYBIND11_MODULE(_core, m) {
  py::class_<vkgs::Renderer>(m, "Renderer")
      .def(py::init<>())
      .def_property_readonly("device_name", &vkgs::Renderer::device_name)
      .def_property_readonly("graphics_queue_index", &vkgs::Renderer::graphics_queue_index)
      .def_property_readonly("compute_queue_index", &vkgs::Renderer::compute_queue_index)
      .def_property_readonly("transfer_queue_index", &vkgs::Renderer::transfer_queue_index)
      .def("load_from_ply", &vkgs::Renderer::LoadFromPly)
      .def("draw", [](vkgs::Renderer& renderer, vkgs::GaussianSplats splats, py::array_t<float> py_view,
                      py::array_t<float> py_projection, uint32_t width, uint32_t height,
                      py::array_t<float> py_background, float eps2d, int sh_degree, py::array_t<uint8_t> dst) {
        const auto* background_ptr = static_cast<const float*>(py_background.request().ptr);
        const auto* view_ptr = static_cast<const float*>(py_view.request().ptr);
        const auto* projection_ptr = static_cast<const float*>(py_projection.request().ptr);
        auto* dst_ptr = static_cast<uint8_t*>(dst.request().ptr);

        // row-major data to column-major
        std::vector<float> view(16);
        for (int r = 0; r < 4; ++r) {
          for (int c = 0; c < 4; ++c) {
            view[c * 4 + r] = view_ptr[r * 4 + c];
          }
        }

        std::vector<float> projection(16);
        for (int r = 0; r < 4; ++r) {
          for (int c = 0; c < 4; ++c) {
            projection[c * 4 + r] = projection_ptr[r * 4 + c];
          }
        }

        return renderer.Draw(splats, view.data(), projection.data(), width, height, background_ptr, eps2d, sh_degree,
                             dst_ptr);
      });

  py::class_<vkgs::GaussianSplats>(m, "GaussianSplats")
      .def(py::init<>())
      .def_property_readonly("size", &vkgs::GaussianSplats::size);

  py::class_<vkgs::RenderedImage>(m, "RenderedImage").def(py::init<>()).def("wait", &vkgs::RenderedImage::Wait);
}
