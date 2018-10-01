#pragma once

#include "sophus.hh"
#include "viewer/primitives/primitive.hh"

#include "geometry/shapes/halfspace.hh"

#include <vector>

namespace viewer {
using Vec3 = Eigen::Vector3d;
using Vec4 = Eigen::Vector4d;

struct Axes {
  SE3 axes_from_world;
  double scale = 1.0;
};

struct Line {
  Vec3 start;
  Vec3 end;
  Vec4 color = Vec4::Ones();
  double width = 2.6;
};

struct Ray {
  Vec3 origin;
  Vec3 direction;
  double length = 1.0;
  Vec4 color = Vec4::Ones();
  double width = 1.0;
};

struct Points {
  std::vector<Vec3> points;
  Vec4 color = Vec4::Ones();
  double size = 1.0;
};

struct Point {
  Vec3 point;
  Vec4 color = Vec4(1.0, 1.0, 0.0, 1.0);
  double size = 1.0;
};

struct ColoredPoints {
  std::vector<Vec3> points;
  std::vector<Vec4> colors;
  double size = 1.0;
};

struct Points2d {
  using Vec2 = Eigen::Vector2d;

  std::vector<Vec2> points;
  Vec4 color = Vec4::Ones();
  double size = 1.0;
  double z_offset = 0.0;
};

struct Sphere {
  Vec3 center;
  double radius;
  Vec4 color = Vec4(0.0, 1.0, 0.0, 1.0);
};

struct Plane {
  geometry::shapes::Plane plane;
  double line_spacing = 1.0;
  Vec4 color = Vec4(0.8, 0.8, 0.8, 0.8);
};

struct AxisAlignedBox {
  Vec3 lower;
  Vec3 upper;
  Vec4 color = Vec4(1.0, 0.0, 1.0, 0.6);
};

struct Polygon {
  std::vector<Vec3, Eigen::aligned_allocator<Vec3>> points;
  double width = 3.0;
  double height = 1.0;
  Vec4 color = Vec4(1.0, 0.0, 1.0, 0.6);
};

void draw_axes(const Axes &axes);

void draw_lines(const std::vector<Line> &lines);

void draw_plane_grid(const Plane &plane);

void draw_points(const Points &points);

void draw_colored_points(const ColoredPoints &points);

void draw_points2d(const Points2d &points);

void draw_billboard_circle(const Sphere &billboard_circle);

void draw_point(const Point &point);

void draw_polygon(const Polygon &polygon);

}  // namespace viewer
