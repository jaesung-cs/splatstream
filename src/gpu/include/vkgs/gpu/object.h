#ifndef VKGS_GPU_OBJECT_H
#define VKGS_GPU_OBJECT_H

#include <memory>

#include "export_api.h"
#include "device.h"

namespace vkgs {
namespace gpu {

class VKGS_GPU_API Object : public std::enable_shared_from_this<Object> {
 public:
  Object();
  virtual ~Object();

  /**
   * Keep this object alive in the currently bound task.
   * Raise exception if no task is bound.
   */
  void Keep();

 protected:
  // Hold device so that device is not destroyed before objects are destroyed.
  Device device_;
};

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_OBJECT_H
