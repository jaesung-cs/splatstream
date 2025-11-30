#ifndef VKGS_COMMON_SHARED_ACCESSOR_H
#define VKGS_COMMON_SHARED_ACCESSOR_H

#include <memory>
#include <utility>

namespace vkgs {

/**
 * A wrapper class that provides a shared pointer to an object.
 * Object is created by Create() function.
 * Object is accessed through operator->().
 * For an object class Foo and an implementation class FooImpl,
 * Derive SharedAccessor<Foo, FooImpl> to create a shared accessor.
 * Example:
 * ```cpp
 * class FooImpl {
 *  public:
 *   FooImpl(int x, float y);
 *   void bar();
 * };
 *
 * class Foo : public SharedAccessor<Foo, FooImpl> {};
 *
 * Foo foo = Foo::Create(1, 2.0f);
 * foo->bar();
 * ```
 */
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

  static ObjectType FromPtr(std::shared_ptr<InstanceType> instance) {
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

  auto impl() const noexcept { return instance_; }

 private:
  std::shared_ptr<InstanceType> instance_;
};

}  // namespace vkgs

#endif  // VKGS_COMMON_SHARED_ACCESSOR_H
