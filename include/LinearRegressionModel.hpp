#ifndef _LINEAR_REGRESSION_MODEL_HPP_
#define _LINEAR_REGRESSION_MODEL_HPP_

#include <Eigen/Dense>
#include <cassert>  // for runtime checks
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

using namespace std;
using namespace Eigen;

inline void normalizeFeatures(MatrixXd &X, vector<double> &mean_out,
                              vector<double> &std_out) {
  int rows = X.rows();
  int cols = X.cols();

  mean_out.resize(cols - 1);
  std_out.resize(cols - 1);

  for (int j = 0; j < cols - 1; ++j) {
    auto col = X.col(j).array();
    double mean = col.mean();
    double sq_sum = (col - mean).square().sum();
    double stddev = std::sqrt(sq_sum / rows);

    if (stddev < 1e-8) stddev = 1.0;

    mean_out[j] = mean;
    std_out[j] = stddev;

    X.col(j) = (col - mean) / stddev;
  }
}

inline double toNormalized(double value, double mean, double stddev) {
  if (stddev < 1e-8) return 0.0;
  return (value - mean) / stddev;
}

inline VectorXd linearRegressionFit(const MatrixXd &X, const VectorXd &y) {
  return X.householderQr().solve(y);
}

inline void evaluateModel(const VectorXd &y_pred, const VectorXd &y_true,
                          const string &label) {
  const int n = y_true.size();
  if (n == 0 || y_pred.size() != n) {
    std::cerr << "Error: input size mismatch or empty vectors.\n";
    return;
  }

  double mae = 0.0, mse = 0.0, maxae = 0.0;
  double y_mean = y_true.mean();
  double ss_tot = 0.0, ss_res = 0.0;
  vector<double> abs_errors;
  abs_errors.reserve(n);

  double mape = 0.0;
  const double eps = 1e-8;  // 避免除以 0

  for (int i = 0; i < n; ++i) {
    double error = y_pred(i) - y_true(i);
    double abs_error = std::abs(error);

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
  double rmse = std::sqrt(mse);
  double r2 = (ss_tot > eps) ? (1.0 - ss_res / ss_tot) : 0.0;

  std::nth_element(abs_errors.begin(), abs_errors.begin() + n / 2, abs_errors.end());
  double medae = abs_errors[n / 2];

  // ===== 評估結果輸出 =====
  cout << "\n[" << label << " model evaluation]\n";
  cout << "-------------------------------\n";
  cout << "MAE     = " << mae   << " ns  (Mean Absolute Error)\n";
  cout << "RMSE    = " << rmse  << " ns  (Root Mean Square Error)\n";
  cout << "MAPE    = " << mape  << " %   (Mean Absolute Percentage Error)\n";
  cout << "MedAE   = " << medae << " ns  (Median Absolute Error)\n";
  cout << "MaxAE   = " << maxae << " ns  (Maximum Absolute Error)\n";
  cout << "R^2     = " << r2    << "     (Coefficient of Determination)\n";
  cout << "-------------------------------\n";
}

inline double predict3(const VectorXd &a, double x1, double x2, double x3) {
  // assert(a.size() == 3);
  return a(0) * x1 + a(1) * x2 + a(2) * x3;
}

inline double predict5(const VectorXd &a, double x1, double x2, double x3,
                       double x4, double x5) {
  // assert(a.size() == 5);
  return a(0) * x1 + a(1) * x2 + a(2) * x3 + a(3) * x4 + a(4) * x5;
}
inline double predict11(const VectorXd &a, double x1, double x2, double x3,
                        double x4, double x5, double x6, double x7, double x8,
                        double x9, double x10, double x11) {
  // assert(a.size() == 11);
  return a(0) * x1 + a(1) * x2 + a(2) * x3 + a(3) * x4 + a(4) * x5 + a(5) * x6 +
         a(6) * x7 + a(7) * x8 + a(8) * x9 + a(9) * x10 + a(10) * x11;
}
// 取得平均值
double computeMean(const Eigen::VectorXd &v) { return v.mean(); }

// 取得中位數
double computeMedian(Eigen::VectorXd v) {
  std::vector<double> data(v.data(), v.data() + v.size());
  std::nth_element(data.begin(), data.begin() + data.size() / 2, data.end());

  if (data.size() % 2 == 1) {
    return data[data.size() / 2];
  } else {
    auto mid1 = data.begin() + data.size() / 2 - 1;
    auto mid2 = data.begin() + data.size() / 2;
    std::nth_element(data.begin(), mid1, data.end());
    return (*mid1 + *mid2) / 2.0;
  }
}

// 取得第 p 百分位數（例如 99th）
double computePercentile(Eigen::VectorXd v, double p) {
  std::vector<double> data(v.data(), v.data() + v.size());
  std::sort(data.begin(), data.end());

  if (data.empty()) return 0.0;

  double rank = p * (data.size() - 1);  // p ∈ [0.0, 1.0]
  size_t lower_idx = static_cast<size_t>(std::floor(rank));
  size_t upper_idx = static_cast<size_t>(std::ceil(rank));

  if (lower_idx == upper_idx) {
    return data[lower_idx];
  } else {
    double weight = rank - lower_idx;
    return data[lower_idx] * (1.0 - weight) + data[upper_idx] * weight;
  }
}
#endif  // LINEAR_REGRESSION_MODEL_HPP
