#ifndef VKGS_COMMON_HANDLE_H
#define VKGS_COMMON_HANDLE_H

#include <memory>
#include <utility>

namespace vkgs {

template <typename T, typename... Args>
concept HasInit = requires(T cls) { cls.__init__(std::declval<Args>()...); };

template <typename T>
concept HasDel = requires(T cls) { cls.__del__(); };

class AnyHandle;

template <typename HandleType, typename ImplType>
class EnableHandleFromThis;

/**
 * A handle for python-like variables: shallow copied handle, lifetime management.
 */
template <typename HandleType, typename ImplType>
class Handle {
  friend class AnyHandle;
  friend class EnableHandleFromThis<HandleType, ImplType>;

 private:
  using _ImplType = ImplType;

 public:
  // Weak handle with weak pointer. Useful for resolving dangling pointers.
  class Weak {
   public:
    Weak() = default;
    Weak(const HandleType& h) : ptr_(h.ptr_), impl_(h.ptr_.get()) {}

    bool expired() const noexcept { return ptr_.expired(); }
    HandleType lock() const noexcept { return ptr_.expired() ? HandleType() : CreateHandle(ptr_.lock()); }

    template <typename T>
    operator T() const {
      // ImplType is an incomplete class, while HandleType isn't.
      // Create a volatile HandleType, and convert to T.
      // This typecasting is valid in __del__() where the weak pointer isn't valid but the content is valid yet.
      HandleType handle;
      handle.impl_ = impl_;
      return static_cast<T>(handle);
    }

    auto operator->() {
      HandleType handle;
      handle.impl_ = impl_;
      return handle.operator->();
    }

   protected:
    ImplType* impl_;

   private:
    std::weak_ptr<ImplType> ptr_;
  };

  static HandleType CreateHandle(std::shared_ptr<ImplType> impl) {
    HandleType handle;
    handle.ptr_ = impl;
    handle.impl_ = handle.ptr_.get();
    return handle;
  }

 protected:
  template <typename T, typename... Args>
  static auto Make(Args&&... args) {
    std::shared_ptr<T> impl(new T(), [](T* r) {
      // Call __del__() if possible.
      if constexpr (HasDel<T>) {
        r->__del__();
      }
      delete r;
    });

    // When creating a concrete class object, call __init__() if possible.
    // This is to make HandleFromThis() valid while constructing the object.
    if constexpr (HasInit<T, Args...>) {
      impl->__init__(std::forward<Args>(args)...);
    } else {
      static_assert(sizeof...(args) == 0, "Make called with arguments but such __init__ is not defined.");
    }

    return CreateHandle(impl);
  }

 public:
  Handle() = default;
  ~Handle() = default;

  Handle(const Handle& rhs) : ptr_(rhs.ptr_), impl_(rhs.impl_) {}
  Handle& operator=(const Handle& rhs) {
    ptr_ = rhs.ptr_;
    impl_ = rhs.impl_;
    return *this;
  }

  Handle(Handle&& rhs) noexcept : ptr_(std::move(rhs.ptr_)), impl_(rhs.impl_) { rhs.impl_ = nullptr; }
  Handle& operator=(Handle&& rhs) noexcept {
    if (this != &rhs) {
      ptr_ = std::move(rhs.ptr_);
      impl_ = rhs.impl_;
      rhs.impl_ = nullptr;
    }
    return *this;
  }

  template <typename T>
  explicit operator T() const {
    return static_cast<T>(*impl_);
  }

  operator bool() const noexcept { return static_cast<bool>(ptr_); }

  void reset() {
    ptr_.reset();
    impl_ = nullptr;
  }

  AnyHandle RemoveType() const;
  operator AnyHandle() const;

  auto impl() const noexcept { return ptr_; }

 protected:
  ImplType* impl_ = nullptr;

 private:
  std::shared_ptr<ImplType> ptr_;
};

class AnyHandle {
 public:
  template <typename T>
  AnyHandle(std::shared_ptr<T> ptr) : ptr_(ptr) {}

  template <typename HandleType>
  HandleType To() {
    return Handle<HandleType, typename HandleType::_ImplType>::CreateHandle(
        std::static_pointer_cast<typename HandleType::_ImplType>(ptr_));
  }

 private:
  std::shared_ptr<void> ptr_;
};

template <typename HandleType, typename ImplType>
AnyHandle Handle<HandleType, ImplType>::RemoveType() const {
  return AnyHandle(ptr_);
}

template <typename HandleType, typename ImplType>
Handle<HandleType, ImplType>::operator AnyHandle() const {
  return RemoveType();
}

// Add HandleFromThis()
// Valid only outside constructor/destructor.
template <typename HandleType, typename ImplType>
class EnableHandleFromThis : public std::enable_shared_from_this<EnableHandleFromThis<HandleType, ImplType>> {
 public:
  virtual ~EnableHandleFromThis() = default;

  HandleType HandleFromThis() {
    auto p = this->shared_from_this();
    auto ptr = std::static_pointer_cast<ImplType>(p);
    return Handle<HandleType, ImplType>::CreateHandle(ptr);
  }
};

}  // namespace vkgs

#endif  // VKGS_COMMON_HANDLE_H
