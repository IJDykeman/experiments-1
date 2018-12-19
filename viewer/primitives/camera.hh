#pragma once

#include "viewer/gl_renderable_buffer.hh"

#include "sophus.hh"

#include <mutex>
#include <opencv2/opencv.hpp>

namespace viewer {

class Camera final {
 public:
  Camera() : back_buffer_(GlSize(640, 640)) {
  }

  void draw();

  void set_world_from_camera(const SE3& world_from_camera) {
    const std::lock_guard<std::mutex> lk(draw_mutex_);
    world_from_camera_ = world_from_camera;
  }

  cv::Mat extract_image() const {
    // TODO: block until an image is created
    while (!have_image()) {
    }
    const std::lock_guard<std::mutex> lk(draw_mutex_);
    return image_;
  }

  bool have_image() const {
    const std::lock_guard<std::mutex> lk(draw_mutex_);
    return have_image_;
  }

  void prepare_view() const;

 private:
  mutable GlRenderableBuffer back_buffer_;

  mutable std::mutex draw_mutex_;
  cv::Mat capture_framebuffer() const;

  SE3 world_from_camera_;
  bool have_image_ = false;
  cv::Mat image_;
};
}  // namespace viewer