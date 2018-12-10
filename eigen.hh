#pragma once

#include <Eigen/Dense>
#include <Eigen/StdVector>

template <int ROWS>
using VecNd = Eigen::Matrix<double, ROWS, 1>;

template <int ROWS, int COLS>
using MatNd = Eigen::Matrix<double, ROWS, COLS>;

template <int ROWS>
using SquareMatNd = Eigen::Matrix<double, ROWS, ROWS>;

template <typename Eig>
using StdVector = std::vector<Eig, Eigen::aligned_allocator<Eig>>;

namespace jcc {
using Vec1 = VecNd<1>;
using Vec2 = Eigen::Vector2d;
using Vec3 = Eigen::Vector3d;
using Vec4 = Eigen::Vector4d;

}  // namespace jcc
