#include "eigen.hh"

#include <Eigen/Dense>

namespace jcc {

template <int a_rows, int b_rows, int cols>
MatNd<a_rows + b_rows, cols> vstack(const MatNd<a_rows, cols>& a, const MatNd<b_rows, cols>& b) {
  MatNd<a_rows + b_rows, cols> ab;
  ab << a, b;
  return ab;
}

template <int a_cols, int b_cols, int rows>
MatNd<rows, a_cols + b_cols> hstack(const MatNd<rows, a_cols>& a, const MatNd<rows, b_cols>& b) {
  MatNd<rows, a_cols + b_cols> ab;
  ab.template leftCols<a_cols>()  = a;
  ab.template rightCols<b_cols>() = b;
  return ab;
}
}