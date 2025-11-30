#ifndef VKGS_GPU_OBJECT_H
#define VKGS_GPU_OBJECT_H

#include <memory>

#include "export_api.h"

namespace vkgs {
namespace gpu {

class Device;

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
  std::shared_ptr<Device> device_;
};

template <typename ObjectType, typename InstanceType>
class SharedAccessor {
 public:
  template <typename... Args>
  static ObjectType Create(Args&&... args) {
    auto instance = std::make_shared<InstanceType>(std::forward<Args>(args)...);
    ObjectType object;
    object.instance_ = instance;
    return object;
  }

 public:
  SharedAccessor() = default;
  virtual ~SharedAccessor() = default;

  void reset() noexcept { instance_ = nullptr; }
  operator bool() const noexcept { return instance_ != nullptr; }
  bool operator!() const noexcept { return instance_ == nullptr; }
  auto operator->() const noexcept { return instance_.get(); }

  template <typename T>
  operator T() const noexcept {
    return static_cast<T>(*instance_.get());
  }

 private:
  std::shared_ptr<InstanceType> instance_;
};

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_OBJECT_H
