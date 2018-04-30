// inc order weird for these guys
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "viewer/window_manager.hh"
#include "window_3d.hh"

#include "eigen_helpers.hh"
#include "gl_aliases.hh"

#include "primitives/simple_geometry_primitives.hh"

#include <thread>

namespace gl_viewer {
namespace {
void pre_render() {
  //
  // Flag soup
  //

  glShadeModel(GL_SMOOTH);

  // Check depth when rendering
  glEnable(GL_DEPTH_TEST);

  // Turn on lighting
  // glEnable(GL_LIGHTING);

  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_BLEND);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
  glEnable(GL_LINE_SMOOTH);
  glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
}
}  // namespace

void View3D::apply() {
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  const Eigen::AngleAxisd az_rot(azimuth, Vec3::UnitY());
  const Eigen::AngleAxisd elev_rot(elevation, Vec3::UnitX());
  const Eigen::Quaterniond q(elev_rot * az_rot);
  const SE3 instantaneous_rotation(SO3(q), Vec3::Zero());
  const SE3 offset(SE3(SO3(), Vec3(0.0, 0.0, -1.0)));
  glTransform(camera_from_target.inverse() * instantaneous_rotation);
  glScaled(zoom, zoom, zoom);

  draw_axes({SE3(), 0.5});
  glTransform(target_from_world * SE3(SO3::exp(Vec3(0.0, 0.0, 3.1415)), Vec3::Zero()));
  draw_axes({SE3(), 1.5});
  simulate();
}

void View3D::simulate() {
  const double t_now = glfwGetTime();
  const double dt = t_now - last_update_time;
  last_update_time = t_now;

  const VecNd<6> delta = jcc::vstack(velocity, angular_velocity) * dt;
  // camera_from_target   = SE3::exp(delta) * camera_from_target;
  target_from_world = SE3::exp(delta) * target_from_world;

  constexpr double translation_damping = 0.9;
  constexpr double rotation_damping = 0.9;

  velocity *= translation_damping;
  angular_velocity *= rotation_damping;
}

void Window3D::spin_until_step() {
  while (!should_step_ && !should_continue_) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  if (should_continue_) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  if (should_step_) {
    should_step_ = false;
  }
}

void Window3D::on_key(int key, int scancode, int action, int mods) {
  const std::lock_guard<std::mutex> lk(behavior_mutex_);

  if (action == GLFW_PRESS) {
    if (key == static_cast<int>('N')) {
      should_step_ = true;
    }

    if (key == static_cast<int>('C')) {
      should_continue_ = !should_continue_;
    }
  }
}
void Window3D::on_mouse_button(int button, int action, int mods) {
  const std::lock_guard<std::mutex> lk(behavior_mutex_);

  mouse_pos_last_click_ = mouse_pos();
}

void Window3D::on_mouse_move(const WindowPoint &mouse_pos) {
  const std::lock_guard<std::mutex> lk(behavior_mutex_);

  if (left_mouse_held()) {
    const Vec2 motion = mouse_pos.point - mouse_pos_last_click_.point;
    mouse_pos_last_click_ = mouse_pos;

    view_.azimuth += motion(0) * 0.005;
    view_.elevation += motion(1) * 0.005;

    if (view_.azimuth > M_PI) {
      view_.azimuth = -M_PI;
    } else if (view_.azimuth < -M_PI) {
      view_.azimuth = M_PI;
    }

    if (view_.elevation > M_PI_2) {
      view_.elevation = M_PI_2;
    } else if (view_.elevation < -M_PI_2) {
      view_.elevation = -M_PI_2;
    }
  }

  if (right_mouse_held()) {
    const Vec2 motion = mouse_pos.point - mouse_pos_last_click_.point;
    mouse_pos_last_click_ = mouse_pos;

    const Vec3 motion_camera_frame(motion.x(), -motion.y(), 0.0);

    const Eigen::AngleAxisd az_rot(view_.azimuth, Vec3::UnitY());
    const Eigen::AngleAxisd elev_rot(view_.elevation, Vec3::UnitX());
    const Eigen::Quaterniond q(elev_rot * az_rot);
    const SE3 instantaneous_rotation(SO3(q), Vec3::Zero());

    // const double motion_scaling = view_.camera_from_target.translation().norm();
    // std::cout << motion_scaling << std::endl;
    // std::cout << "z:" << view_.zoom << std::endl;

    const double inv_zoom = 1.0 / view_.zoom;

    // const Vec3 motion_target_frame = instantaneous_rotation.inverse() * (motion_camera_frame * view_.zoom * 15.0);
    const Vec3 motion_target_frame = instantaneous_rotation.inverse() * (motion_camera_frame * inv_zoom * 0.05);
    view_.target_from_world.translation() += motion_target_frame;
  }
}

void Window3D::on_scroll(const double amount) {
  const std::lock_guard<std::mutex> lk(behavior_mutex_);

  view_.zoom *= std::exp(0.1 * amount);
  if (view_.zoom <= 0.0001) {
    view_.zoom = 0.0001;
  }
}

void Window3D::resize(const GlSize &gl_size) {
  const std::lock_guard<std::mutex> lk(behavior_mutex_);

  glViewport(0, 0, gl_size.width, gl_size.height);
  gl_size_ = gl_size;
}

void Window3D::render() {
  const std::lock_guard<std::mutex> lk(behavior_mutex_);
  pre_render();

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(60, (static_cast<double>(gl_size_.width) / static_cast<double>(gl_size_.height)), 0.001, 1000.0);

  // Update projection
  projection_ = Projection::get_from_current();

  for (const auto &primitive : primitives_) {
    primitive->draw();
  }

  apply_keys_to_view();
  view_.apply();

  glFlush();
  glFinish();
}

void Window3D::apply_keys_to_view() {
  const auto keys = held_keys();
  const double acceleration = 0.005;

  Vec3 delta_vel = Vec3::Zero();
  for (const auto &key_element : keys) {
    const bool held = key_element.second;
    const int key = key_element.first;

    if (!held) {
      continue;
    }

    switch (key) {
      case (static_cast<int>('W')):
        delta_vel(2) += acceleration;
        break;

      case (static_cast<int>('A')):
        delta_vel(0) += acceleration;
        break;

      case (static_cast<int>('S')):
        delta_vel(2) -= acceleration;

        break;
      case (static_cast<int>('D')):
        delta_vel(0) -= acceleration;
        break;

      case (static_cast<int>('C')):
        delta_vel(1) += acceleration;
        break;

      case (static_cast<int>('Z')):
        delta_vel(1) -= acceleration;
        break;

      case (static_cast<int>('R')):
        view_.target_from_world = SE3();
        break;
    }
  }

  view_.velocity -= delta_vel;
}

struct Window3DGlobalState {
  std::map<std::string, std::shared_ptr<Window3D>> windows;
};

Window3DGlobalState window_3d_state;

std::shared_ptr<Window3D> get_window3d(const std::string &title) {
  const auto it = window_3d_state.windows.find(title);
  if (it != window_3d_state.windows.end()) {
    return it->second;
  } else {
    auto window = std::make_shared<Window3D>();
    window_3d_state.windows[title] = window;
    WindowManager::register_window(GlSize(640, 640), window, title);
    return window;
  }
}
}  // namespace gl_viewer
