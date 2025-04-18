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
  // assert(y_pred.size() == y_true.size());
  int n = y_true.size();
  double mae = 0.0, mse = 0.0;
  double y_mean = y_true.mean();

  for (int i = 0; i < n; ++i) {
    double error = y_pred(i) - y_true(i);
    mae += std::abs(error);
    mse += error * error;
  }

  mae /= n;
  mse /= n;
  double rmse = std::sqrt(mse);

  cout << "\n[" << label << " model evaluation]" << endl;
  cout << "MAE  = " << mae << " ns" << endl;
  cout << "RMSE = " << rmse << " ns" << endl;
}

inline double predict3(const VectorXd &a, double x1, double x2, double x3) {
  // assert(a.size() == 4);
  return a(0) * x1 + a(1) * x2 + a(2) * x3;
}

inline double predict5(const VectorXd &a, double x1, double x2, double x3,
                       double x4, double x5) {
  // assert(a.size() == 6);
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
