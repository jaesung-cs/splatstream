#ifndef VKGS_COMMON_SAMPLED_MOVING_WINDOW_H
#define VKGS_COMMON_SAMPLED_MOVING_WINDOW_H

#include <deque>

namespace vkgs {

class SampledMovingWindow {
 private:
  struct Point {
    float time;
    float value;
  };

 public:
  SampledMovingWindow() = default;

  SampledMovingWindow(float window_size, float sample_interval)
      : window_size_(window_size), sample_interval_(sample_interval) {}

  ~SampledMovingWindow() = default;

  void add(float time, float value) {
    while (!points_.empty() && points_.front().time < time - window_size_) {
      points_.pop_front();
    }
    if (time - sample_time_ > sample_interval_) {
      if (sample_count_ > 0) points_.push_back({sample_time_, sample_value_ / sample_count_});
      sample_value_ = value;
      sample_count_ = 1;
      sample_time_ = time;
    } else {
      sample_value_ += value;
      sample_count_++;
    }
    last_time_ = time;
  }

  std::deque<Point> points() const {
    auto result = points_;
    if (sample_count_ > 0) {
      result.push_back({last_time_, sample_value_ / sample_count_});
    }
    return result;
  }

 private:
  float window_size_ = 0.0f;
  float sample_interval_ = 0.0f;
  float last_time_ = 0.0f;
  float sample_time_ = 0.0f;
  float sample_value_ = 0.0f;
  int sample_count_ = 0;
  std::deque<Point> points_;
};

}  // namespace vkgs

#endif  // VKGS_COMMON_SAMPLED_MOVING_WINDOW_H
