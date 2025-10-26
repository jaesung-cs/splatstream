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
      .def("create_gaussian_splats",
           [](vkgs::Renderer& renderer, py::array_t<float> means, py::array_t<float> quats, py::array_t<float> scales,
              intptr_t sh_ptr, py::array_t<float> opacity, int sh_degree) {
             size_t N = means.shape(0);
             const auto* means_ptr = static_cast<const float*>(means.request().ptr);
             const auto* quats_ptr = static_cast<const float*>(quats.request().ptr);
             const auto* scales_ptr = static_cast<const float*>(scales.request().ptr);
             const auto* sh_u16_ptr = reinterpret_cast<const uint16_t*>(sh_ptr);
             const auto* opacity_ptr = static_cast<const float*>(opacity.request().ptr);
             return renderer.CreateGaussianSplats(N, means_ptr, quats_ptr, scales_ptr, sh_u16_ptr, opacity_ptr,
                                                  sh_degree);
           })
      .def("draw", [](vkgs::Renderer& renderer, vkgs::GaussianSplats splats, py::array_t<float> view,
                      py::array_t<float> projection, uint32_t width, uint32_t height, py::array_t<float> background,
                      float eps2d, int sh_degree, py::array_t<uint8_t> dst) {
        const auto* background_ptr = static_cast<const float*>(background.request().ptr);
        const auto* view_ptr = static_cast<const float*>(view.request().ptr);
        const auto* projection_ptr = static_cast<const float*>(projection.request().ptr);
        auto* dst_ptr = static_cast<uint8_t*>(dst.request().ptr);

        vkgs::DrawOptions draw_options = {};
        // row-major data to column-major
        for (int r = 0; r < 4; ++r) {
          for (int c = 0; c < 4; ++c) {
            draw_options.view[c * 4 + r] = view_ptr[r * 4 + c];
            draw_options.projection[c * 4 + r] = projection_ptr[r * 4 + c];
          }
        }
        draw_options.width = width;
        draw_options.height = height;
        std::memcpy(draw_options.background, background_ptr, 3 * sizeof(float));
        draw_options.eps2d = eps2d;
        draw_options.sh_degree = sh_degree;
        return renderer.Draw(splats, draw_options, dst_ptr);
      });

  py::class_<vkgs::GaussianSplats>(m, "GaussianSplats").def_property_readonly("size", &vkgs::GaussianSplats::size);

  py::class_<vkgs::RenderedImage>(m, "RenderedImage").def("wait", &vkgs::RenderedImage::Wait);
}
