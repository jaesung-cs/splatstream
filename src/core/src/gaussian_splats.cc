#include "vkgs/core/gaussian_splats.h"

#include "vkgs/gpu/buffer.h"

namespace vkgs {
namespace core {

GaussianSplats::GaussianSplats(size_t size, std::shared_ptr<gpu::Buffer> position, std::shared_ptr<gpu::Buffer> cov3d,
                               std::shared_ptr<gpu::Buffer> sh, std::shared_ptr<gpu::Buffer> opacity,
                               std::shared_ptr<gpu::Buffer> index_buffer)
    : size_(size), position_(position), cov3d_(cov3d), sh_(sh), opacity_(opacity), index_buffer_(index_buffer) {}

GaussianSplats::~GaussianSplats() = default;

}  // namespace core
}  // namespace vkgs
