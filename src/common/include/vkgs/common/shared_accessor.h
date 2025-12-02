#ifndef VKGS_COMMON_SHARED_ACCESSOR_H
#define VKGS_COMMON_SHARED_ACCESSOR_H

#include <memory>
#include <utility>

namespace vkgs {

/**
 * @brief A wrapper class that provides a shared pointer to an object.
 *
 * @details This class provides a convenient way to wrap an implementation class
 *          with a shared pointer. Objects are created using the Create() function
 *          and accessed through operator->().
 *
 *          For an object class Foo and an implementation class FooImpl,
 *          derive SharedAccessor<Foo, FooImpl> to create a shared accessor.
 *
 * @tparam ObjectType The public-facing object type that users interact with.
 * @tparam InstanceType The implementation type that contains the actual logic.
 *
 * @par Basic Usage Example:
 * @code{.cpp}
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
 * @endcode
 *
 * @par Advanced Usage with Type Deduction:
 * The Create() function will fail to deduce types when using initializer_list
 * or designated initialized structs. To enable type deduction for these cases,
 * explicitly define a Create() overload:
 *
 * @code{.cpp}
 * struct FooCreateInfo { int x; float y; };
 *
 * class FooImpl {
 *  public:
 *   FooImpl(int x, float y);
 *   FooImpl(const FooCreateInfo& create_info);
 * };
 *
 * class Foo : public SharedAccessor<Foo, FooImpl> {
 *  public:
 *   using Base::Create;                                    // To expose automatically
 *   static Foo Create(const FooCreateInfo& create_info) {  // To explicitly deduce FooCreateInfo
 *     Base::Create(create_info);
 *   }
 * };
 *
 * Foo foo = Foo::Create(1, 2.f);              // Exposed implicitly by FooImpl
 * Foo bar = Foo::Create({.x = 1, .y = 2.f});  // Exposed explicitly by Foo
 * @endcode
 */
template <typename ObjectType, typename InstanceType>
class SharedAccessor {
 protected:
  using Base = SharedAccessor;

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
