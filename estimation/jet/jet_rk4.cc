#include "estimation/jet/jet_rk4.hh"

#include "eigen.hh"

namespace estimation {
namespace jet_filter {
VecNd<3> observe_accel(const SE3 &T_world_from_sensor, const VecNd<6> &eps_dot,
                       const VecNd<6> &eps_ddot, const VecNd<3> &gravity_mpss) {
  const VecNd<3> v = eps_dot.block<3, 1>(0, 0);
  const SO3 R_world_from_sensor = T_world_from_sensor.so3();
  const VecNd<3> w = eps_dot.block<3, 1>(3, 0);
  const MatNd<3, 3> Rvx = SO3::hat((R_world_from_sensor * v));
  const VecNd<3> coriolis = (Rvx * w) + (v.cross((R_world_from_sensor * w)));
  const VecNd<3> centrifugal =
      (T_world_from_sensor.translation()).cross((Rvx * w));
  const MatNd<6, 6> adj = T_world_from_sensor.Adj();
  const VecNd<6> ad_times_inertial_and_euler = adj * eps_ddot;
  const VecNd<3> inertial_and_euler =
      ad_times_inertial_and_euler.block<3, 1>(0, 0);
  const VecNd<3> g_imu_mpss = (R_world_from_sensor.inverse()) * gravity_mpss;
  const VecNd<3> observed_acceleration =
      ((coriolis + centrifugal) + inertial_and_euler) + g_imu_mpss;
  return observed_acceleration;
}
VecNd<24> to_vector(const StateDelta &in_grp) {
  const VecNd<24> out =
      (VecNd<24>() << ((in_grp.eps_dot_error)[0]), ((in_grp.eps_dot_error)[1]),
       ((in_grp.eps_dot_error)[2]), ((in_grp.eps_dot_error)[3]),
       ((in_grp.eps_dot_error)[4]), ((in_grp.eps_dot_error)[5]),
       ((in_grp.gyro_bias_error)[0]), ((in_grp.gyro_bias_error)[1]),
       ((in_grp.gyro_bias_error)[2]), ((in_grp.eps_ddot_error)[0]),
       ((in_grp.eps_ddot_error)[1]), ((in_grp.eps_ddot_error)[2]),
       ((in_grp.eps_ddot_error)[3]), ((in_grp.eps_ddot_error)[4]),
       ((in_grp.eps_ddot_error)[5]), ((in_grp.T_world_from_body_error_log)[0]),
       ((in_grp.T_world_from_body_error_log)[1]),
       ((in_grp.T_world_from_body_error_log)[2]),
       ((in_grp.T_world_from_body_error_log)[3]),
       ((in_grp.T_world_from_body_error_log)[4]),
       ((in_grp.T_world_from_body_error_log)[5]),
       ((in_grp.accel_bias_error)[0]), ((in_grp.accel_bias_error)[1]),
       ((in_grp.accel_bias_error)[2]))
          .finished();
  return out;
}
State operator-(const State &a, const State &b) {
  const State difference =
      State{((a.eps_dot) - (b.eps_dot)), ((a.gyro_bias) - (b.gyro_bias)),
            ((a.eps_ddot) - (b.eps_ddot)),
            ((a.T_world_from_body) * ((b.T_world_from_body).inverse())),
            ((a.accel_bias) - (b.accel_bias))};
  return difference;
}
VecNd<24> compute_delta(const State &a, const State &b) {
  const State difference = a - b;
  const VecNd<6> eps_dot_error = difference.eps_dot;
  const VecNd<3> gyro_bias_error = difference.gyro_bias;
  const VecNd<6> eps_ddot_error = difference.eps_ddot;
  const SE3 T_world_from_body_error = difference.T_world_from_body;
  const VecNd<6> T_world_from_body_error_log =
      SE3::log(T_world_from_body_error);
  const VecNd<3> accel_bias_error = difference.accel_bias;
  const StateDelta delta =
      StateDelta{eps_dot_error, gyro_bias_error, eps_ddot_error,
                 T_world_from_body_error_log, accel_bias_error};
  const VecNd<24> out_vec = to_vector(delta);
  return out_vec;
}
VecNd<3> to_vector(const AccelMeasurementDelta &in_grp) {
  const VecNd<3> out = (VecNd<3>() << ((in_grp.observed_acceleration_error)[0]),
                        ((in_grp.observed_acceleration_error)[1]),
                        ((in_grp.observed_acceleration_error)[2]))
                           .finished();
  return out;
}
AccelMeasurement operator-(const AccelMeasurement &a,
                           const AccelMeasurement &b) {
  const AccelMeasurement difference =
      AccelMeasurement{((a.observed_acceleration) - (b.observed_acceleration))};
  return difference;
}
VecNd<3> compute_delta(const AccelMeasurement &a, const AccelMeasurement &b) {
  const AccelMeasurement difference = a - b;
  const VecNd<3> observed_acceleration_error = difference.observed_acceleration;
  const AccelMeasurementDelta delta =
      AccelMeasurementDelta{observed_acceleration_error};
  const VecNd<3> out_vec = to_vector(delta);
  return out_vec;
}
StateDot compute_qdot(const State &Q, const Parameters &Z) {
  const VecNd<6> eps_dddot = Z.eps_dddot;
  const VecNd<6> eps_dot = Q.eps_dot;
  const VecNd<6> eps_ddot = Q.eps_ddot;
  const VecNd<3> dgyro_bias = Z.dgyro_bias;
  const VecNd<3> daccel_bias = Z.daccel_bias;
  const StateDot Qdot =
      StateDot{eps_ddot, dgyro_bias, eps_dddot, eps_dot, daccel_bias};
  return Qdot;
}
StateDot operator*(const double half_h, const StateDot &K1) {
  const StateDot anon_0d73ec =
      StateDot{(half_h * (K1.eps_ddot)), (half_h * (K1.dgyro_bias)),
               (half_h * (K1.eps_dddot)), (half_h * (K1.eps_dot)),
               (half_h * (K1.daccel_bias))};
  return anon_0d73ec;
}
State operator+(const State &Q, const StateDot &anon_0d73ec) {
  const State Q2 =
      State{((Q.eps_dot) + (anon_0d73ec.eps_ddot)),
            ((Q.gyro_bias) + (anon_0d73ec.dgyro_bias)),
            ((Q.eps_ddot) + (anon_0d73ec.eps_dddot)),
            ((SE3::exp((anon_0d73ec.eps_dot))) * (Q.T_world_from_body)),
            ((Q.accel_bias) + (anon_0d73ec.daccel_bias))};
  return Q2;
}
StateDot operator+(const StateDot &K1, const StateDot &K4) {
  const StateDot anon_cd8673 = StateDot{
      ((K1.eps_ddot) + (K4.eps_ddot)), ((K1.dgyro_bias) + (K4.dgyro_bias)),
      ((K1.eps_dddot) + (K4.eps_dddot)), ((K1.eps_dot) + (K4.eps_dot)),
      ((K1.daccel_bias) + (K4.daccel_bias))};
  return anon_cd8673;
}
State rk4_integrate(const State &Q, const Parameters &Z, const double h) {
  const double half = 0.5;
  const double half_h = h * half;
  const StateDot K1 = compute_qdot(Q, Z);
  const State Q2 = Q + (half_h * K1);
  const StateDot K2 = compute_qdot(Q2, Z);
  const State Q3 = Q + (half_h * K2);
  const StateDot K3 = compute_qdot(Q3, Z);
  const State Q4 = Q + (h * K3);
  const StateDot K4 = compute_qdot(Q4, Z);
  const double two = 2.0;
  const double sixth = 0.166666666667;
  const State Qn = Q + (sixth * ((K1 + K4) + (two * (K2 + K3))));
  return Qn;
}
State operator+(const State &a, const StateDelta &grp_b) {
  const State out = State{
      ((a.eps_dot) + (grp_b.eps_dot_error)),
      ((a.gyro_bias) + (grp_b.gyro_bias_error)),
      ((a.eps_ddot) + (grp_b.eps_ddot_error)),
      ((SE3::exp((grp_b.T_world_from_body_error_log))) * (a.T_world_from_body)),
      ((a.accel_bias) + (grp_b.accel_bias_error))};
  return out;
}
StateDelta from_vector(const VecNd<24> &in_vec) {
  const VecNd<3> anon_832c70 =
      (VecNd<3>() << (in_vec[6]), (in_vec[7]), (in_vec[8])).finished();
  const VecNd<6> anon_223b31 =
      (VecNd<6>() << (in_vec[0]), (in_vec[1]), (in_vec[2]), (in_vec[3]),
       (in_vec[4]), (in_vec[5]))
          .finished();
  const VecNd<6> anon_31338a =
      (VecNd<6>() << (in_vec[9]), (in_vec[10]), (in_vec[11]), (in_vec[12]),
       (in_vec[13]), (in_vec[14]))
          .finished();
  const VecNd<6> anon_3f9026 =
      (VecNd<6>() << (in_vec[15]), (in_vec[16]), (in_vec[17]), (in_vec[18]),
       (in_vec[19]), (in_vec[20]))
          .finished();
  const VecNd<3> anon_36644d =
      (VecNd<3>() << (in_vec[21]), (in_vec[22]), (in_vec[23])).finished();
  const StateDelta out = StateDelta{anon_223b31, anon_832c70, anon_31338a,
                                    anon_3f9026, anon_36644d};
  return out;
}
State apply_delta(const State &a, const VecNd<24> &delta) {
  const StateDelta grp_b = from_vector(delta);
  const State out = a + grp_b;
  return out;
}
AccelMeasurement operator+(const AccelMeasurement &a,
                           const AccelMeasurementDelta &grp_b) {
  const AccelMeasurement out = AccelMeasurement{
      ((a.observed_acceleration) + (grp_b.observed_acceleration_error))};
  return out;
}
AccelMeasurementDelta from_vector(const VecNd<3> &in_vec) {
  const VecNd<3> anon_d627b1 =
      (VecNd<3>() << (in_vec[0]), (in_vec[1]), (in_vec[2])).finished();
  const AccelMeasurementDelta out = AccelMeasurementDelta{anon_d627b1};
  return out;
}
AccelMeasurement apply_delta(const AccelMeasurement &a, const VecNd<3> &delta) {
  const AccelMeasurementDelta grp_b = from_vector(delta);
  const AccelMeasurement out = a + grp_b;
  return out;
}
VecNd<3> observe_gyro(const VecNd<6> &eps_dot, const SE3 &T_world_from_sensor) {
  const SO3 R_world_from_sensor = T_world_from_sensor.so3();
  const VecNd<3> w = eps_dot.block<3, 1>(3, 0);
  return ((R_world_from_sensor.inverse()) * w);
}
} // namespace jet_filter
} // namespace estimation