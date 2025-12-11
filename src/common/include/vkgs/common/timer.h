#ifndef VKGS_COMMON_TIMER_H
#define VKGS_COMMON_TIMER_H

#include <chrono>

namespace vkgs {

class Timer {
 public:
  Timer() { start_time_ = std::chrono::high_resolution_clock::now(); }

  ~Timer() = default;

  void start() { start_time_ = std::chrono::high_resolution_clock::now(); }

  double elapsed() const noexcept {
    return std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - start_time_).count();
  }

 private:
  std::chrono::time_point<std::chrono::high_resolution_clock> start_time_;
};

}  // namespace vkgs

#endif  // VKGS_COMMON_TIMER_H
