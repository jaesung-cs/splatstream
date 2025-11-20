#ifndef VKGS_VIEWER_CONTEXT_H
#define VKGS_VIEWER_CONTEXT_H

#include <memory>

namespace vkgs {
namespace viewer {

class Context;
std::shared_ptr<Context> GetContext();

class Context {
 public:
  Context();
  ~Context();

 private:
};

}  // namespace viewer
}  // namespace vkgs

#endif  // VKGS_VIEWER_CONTEXT_H
