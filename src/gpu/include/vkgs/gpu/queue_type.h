#ifndef VKGS_GPU_QUEUE_TYPE_H
#define VKGS_GPU_QUEUE_TYPE_H

namespace vkgs {
namespace gpu {

enum class QueueType {
  TRANSFER,
  COMPUTE,
  GRAPHICS,
};

}
}  // namespace vkgs

#endif  // VKGS_GPU_QUEUE_TYPE_H
