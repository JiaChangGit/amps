#ifndef LINEAR_REGRESSION_MODEL_HPP
#define LINEAR_REGRESSION_MODEL_HPP

#include <Eigen/Dense>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <tuple>
#include <vector>

inline void normalizeFeatures(Eigen::MatrixXd &X, Eigen::VectorXd &mean_out,
                              Eigen::VectorXd &std_out) {
  const int rows = X.rows();
  const int cols = X.cols();

  mean_out.resize(cols);
  std_out.resize(cols);

  for (int j = 0; j < cols; ++j) {
    const auto col = X.col(j).array();
    const double mean = col.mean();
    const double sq_sum = (col - mean).square().sum();
    double stddev = std::sqrt(sq_sum / rows);

    if (stddev < 1e-8) stddev = 1.0;  // 避免除以零問題

    mean_out(j) = mean;
    std_out(j) = stddev;

    X.col(j) = (col - mean) / stddev;
  }
}

inline double toNormalized(double value, double mean, double stddev) {
  return (stddev < 1e-8) ? 0.0 : (value - mean) / stddev;
}

inline Eigen::VectorXd linearRegressionFit(const Eigen::MatrixXd &X,
                                           const Eigen::VectorXd &y) {
  assert(X.rows() == y.size());
  return X.householderQr().solve(y);
}

inline void evaluateModel(const Eigen::VectorXd &y_pred,
                          const Eigen::VectorXd &y_true,
                          const std::string &label,
                          std::ostream &os = std::cout) {
  const int n = y_true.size();
  if (n == 0 || y_pred.size() != n) {
    std::cerr << "Error: input size mismatch or empty vectors.\n";
    return;
  }

  double mae = 0.0, mse = 0.0, maxae = 0.0;
  const double y_mean = y_true.mean();
  double ss_tot = 0.0, ss_res = 0.0;

  std::vector<double> abs_errors;
  abs_errors.reserve(n);

  double mape = 0.0;
  constexpr double eps = 1e-8;  // 避免除以零

  for (int i = 0; i < n; ++i) {
    const double error = y_pred(i) - y_true(i);
    const double abs_error = std::abs(error);

    mae += abs_error;
    mse += error * error;
    maxae = std::max(maxae, abs_error);
    abs_errors.push_back(abs_error);

    if (std::abs(y_true(i)) > eps) {
      mape += abs_error / std::abs(y_true(i));
    }

    ss_res += error * error;
    ss_tot += (y_true(i) - y_mean) * (y_true(i) - y_mean);
  }

  mae /= n;
  mse /= n;
  mape = (mape / n) * 100.0;
  const double rmse = std::sqrt(mse);
  const double r2 = (ss_tot > eps) ? (1.0 - ss_res / ss_tot) : 0.0;

  std::nth_element(abs_errors.begin(), abs_errors.begin() + n / 2,
                   abs_errors.end());
  const double medae = abs_errors[n / 2];

  os << "\n[" << label << " model evaluation]\n";
  os << "-------------------------------\n";
  os << "MAE     = " << mae << " ns  (Mean Absolute Error)\n";
  os << "RMSE    = " << rmse << " ns  (Root Mean Square Error)\n";
  os << "MAPE    = " << mape << " %   (Mean Absolute Percentage Error)\n";
  os << "MedAE   = " << medae << " ns  (Median Absolute Error)\n";
  os << "MaxAE   = " << maxae << " ns  (Maximum Absolute Error)\n";
  os << "R^2     = " << r2 << "     (Coefficient of Determination)\n";
  os << "-------------------------------\n";
}

inline double predict3(const Eigen::VectorXd &a, double x1, double x2,
                       double x3) {
  assert(a.size() == 3);
  return std::abs(a(0) * x1 + a(1) * x2 + a(2) * x3);
}

inline double predict5(const Eigen::VectorXd &a, double x1, double x2,
                       double x3, double x4, double x5) {
  assert(a.size() == 5);
  return std::abs(a(0) * x1 + a(1) * x2 + a(2) * x3 + a(3) * x4 + a(4) * x5);
}

inline double predict11(const Eigen::VectorXd &a, double x1, double x2,
                        double x3, double x4, double x5, double x6, double x7,
                        double x8, double x9, double x10, double x11) {
  assert(a.size() == 11);
  return std::abs(a(0) * x1 + a(1) * x2 + a(2) * x3 + a(3) * x4 + a(4) * x5 +
                  a(5) * x6 + a(6) * x7 + a(7) * x8 + a(8) * x9 + a(9) * x10 +
                  a(10) * x11);
}

// inline double predict3_poly2(const Eigen::VectorXd &a, double x1, double x2,
//                              double x3) {
//   assert(a.size() == 3);
//   return  std::abs(a(0) * (x1 + x1 * x1) + a(1) * (x2 + x2 * x2) + a(2) * (x3
//   + x3 * x3));
// }

inline double computeMean(const Eigen::VectorXd &v) { return v.mean(); }

inline double computeMedian(Eigen::VectorXd v) {
  const int n = v.size();
  assert(n > 0);

  std::vector<double> data(v.data(), v.data() + n);
  std::nth_element(data.begin(), data.begin() + n / 2, data.end());

  if (n % 2 == 1) {
    return data[n / 2];
  } else {
    std::nth_element(data.begin(), data.begin() + n / 2 - 1, data.end());
    return (data[n / 2] + data[n / 2 - 1]) / 2.0;
  }
}

inline double computePercentile(Eigen::VectorXd v, double p) {
  const int n = v.size();
  assert(n > 0 && p >= 0.0 && p <= 1.0);

  std::vector<double> data(v.data(), v.data() + n);
  std::sort(data.begin(), data.end());

  const double rank = p * (n - 1);
  const size_t lower_idx = static_cast<size_t>(std::floor(rank));
  const size_t upper_idx = static_cast<size_t>(std::ceil(rank));

  if (lower_idx == upper_idx) {
    return data[lower_idx];
  } else {
    const double weight = rank - lower_idx;
    return data[lower_idx] * (1.0 - weight) + data[upper_idx] * weight;
  }
}

inline std::tuple<double, int> get_min_max_time(double predicted_time_pt,
                                                double predicted_time_dbt,
                                                double predicted_time_kset) {
  double min_val = predicted_time_pt;
  int min_id_predict = 0;

  if (predicted_time_dbt < min_val) {
    min_val = predicted_time_dbt;
    min_id_predict = 1;
  }
  if (predicted_time_kset < min_val) {
    min_val = predicted_time_kset;
    min_id_predict = 2;
  }
  return {min_val, min_id_predict};
}

#endif  // LINEAR_REGRESSION_MODEL_HPP
