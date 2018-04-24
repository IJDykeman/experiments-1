#pragma once

#include <atomic>
#include <mutex>

#include "eigen.hh"
#include "primitive.hh"

#include <opencv2/core/eigen.hpp>
#include <opencv2/opencv.hpp>

namespace gl_viewer {

class Image final : public Primitive {
 public:
  Image(const cv::Mat& image, double scale = 1.0, double alpha = 0.7);
  Image(const Eigen::MatrixXd& image, double scale = 1.0, double alpha = 0.7);

  void update_image(const cv::Mat& image, double scale = 1.0);
  void update_image(const Eigen::MatrixXd& image, double scale = 1.0);

  void draw() const override;

 private:
  void update_gl() const;

  cv::Mat      image_;
  double       scale_ = 1.0;
  double       alpha_ = 0.7;


  mutable unsigned int texture_id_;
  mutable bool allocated_texture_ = false;
  mutable std::atomic<bool> to_update_{false};
  mutable std::mutex        draw_mutex_;
};
}  // namespace gl_viewer
