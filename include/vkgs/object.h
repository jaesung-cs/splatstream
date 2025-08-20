#ifndef VKGS_OBJECT_H
#define VKGS_OBJECT_H

#include <vector>

namespace vkgs {

class Object {
 public:
  Object() = default;

  Object(Object* parent) { parent->children_.push_back(this); }

  virtual ~Object() = default;

  void DestroyThis() {
    if (destroyed_) return;

    PreDestroy();

    for (auto child : children_) child->DestroyThis();
    children_.clear();

    Destroy();

    destroyed_ = true;
  }

  virtual void PreDestroy() {}
  virtual void Destroy() = 0;

 private:
  bool destroyed_ = false;
  std::vector<Object*> children_;
};

}  // namespace vkgs

#endif  // VKGS_OBJECT_H
