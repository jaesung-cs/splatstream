#ifndef VKGS_GPU_OBJECT_H
#define VKGS_GPU_OBJECT_H

#include <memory>

#include "export_api.h"

namespace vkgs {
namespace gpu {

class Device;

class VKGS_GPU_API Object : public std::enable_shared_from_this<Object> {
 public:
  Object(std::shared_ptr<Device> device);
  virtual ~Object();

  /**
   * Keep this object alive in the currently bound task.
   * Raise exception if no task is bound.
   */
  void Keep();

 protected:
  std::shared_ptr<Device> device_;
};

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_OBJECT_H
