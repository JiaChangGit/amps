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

/**
 * @brief 對資料矩陣進行欄位（特徵）標準化，使每一欄資料轉為 mean = 0, std = 1。
 *
 * 使用公式：x_j^{(i)} := (x_j^{(i)} - mean_j) / std_j
 * 其中 j 為特徵索引，i 為樣本編號
 *
 * @param X        [in/out]
 * 每列為一筆樣本、每欄為一個特徵，將就地標準化（zero-mean, unit-variance）。
 * @param mean_out [out]    每個特徵的平均值（用於還原或後續推論）。
 * @param std_out  [out]    每個特徵的標準差（用於還原或後續推論）。
 */
inline void normalizeFeatures(Eigen::MatrixXd &X, Eigen::VectorXd &mean_out,
                              Eigen::VectorXd &std_out) {
  const int rows = X.rows();
  const int cols = X.cols();

  mean_out.resize(cols);
  std_out.resize(cols);

  for (int j = 0; j < cols; ++j) {
    // 取得第 j 欄作為 array（方便進行 element-wise 運算）
    const auto col = X.col(j).array();

    // 計算該欄的平均值與中心化後的向量
    const double mean = col.mean();
    const Eigen::ArrayXd centered = col - mean;

    // 計算標準差（使用 population stddev，分母為 N）
    double stddev = std::sqrt(centered.square().sum() / rows);

    // 若 stddev 太小（表示資料無變化），避免除以零或產生數值爆炸
    if (stddev < 1e-8) stddev = 1.0;

    // 儲存該欄的統計資訊
    mean_out(j) = mean;
    std_out(j) = stddev;

    // 將第 j 欄就地標準化（zero-mean, unit-variance）
    X.col(j) = centered / stddev;
  }
}

// JIA bias
// inline void normalizeFeatures(Eigen::MatrixXd &X, Eigen::VectorXd &mean_out,
//                               Eigen::VectorXd &std_out) {
//   int rows = X.rows();
//   int cols = X.cols();

//   mean_out.resize(cols - 1);
//   std_out.resize(cols - 1);

//   for (int j = 0; j < cols - 1; ++j) {
//     auto col = X.col(j).array();
//     double mean = col.mean();
//     double sq_sum = (col - mean).square().sum();
//     double stddev = std::sqrt(sq_sum / rows);

//     if (stddev < 1e-8) stddev = 1.0;

//     mean_out[j] = mean;
//     std_out[j] = stddev;

//     X.col(j) = (col - mean) / stddev;
//   }
// }

/**
 * @brief 將單一特徵值轉換為標準化後的值。
 * 使用公式：normalized_value = (value - mean) / stddev
 * 若 stddev 太小則視為常數特徵，直接回傳 0
 */
inline double toNormalized(double value, double mean, double stddev) {
  return (stddev < 1e-8) ? 0.0 : (value - mean) / stddev;
}

/**
 * @brief 計算線性回歸模型係數
 * 利用最小平方法 Least Squares 解以下問題：
 *    beta = argmin ||X * beta - y||^2
 * 解法：beta = (X^T X)^-1 X^T y，這裡用 Householder QR 分解求解
 */
inline Eigen::VectorXd linearRegressionFit(const Eigen::MatrixXd &X,
                                           const Eigen::VectorXd &y) {
  assert(X.rows() == y.size());
  return X.householderQr().solve(y);
}

/**
 * @brief 評估模型預測結果的誤差，並輸出常見統計指標：
 * - MAE, RMSE, MAPE, MedAE, MaxAE, R^2
 *
 * RMSE = sqrt( MSE )
 * MAE = mean( |y_true - y_pred| )
 * R^2 = 1 - sum((y_i - pred_i)^2) / sum((y_i - mean_y)^2)
 */
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
  return std::
      abs(a(0) * x1 + a(1) * x2 +
          a(2) * x3 /*+
a(3)*/);            /* + a(3) JIA bias */
}

inline double predict5(const Eigen::VectorXd &a, double x1, double x2,
                       double x3, double x4, double x5) {
  assert(a.size() == 5);
  return std::
      abs(a(0) * x1 + a(1) * x2 + a(2) * x3 + a(3) * x4 + a(4) * x5 /*+
a(5)*/); /* + a(5) JIA bias */
}

inline double predict11(const Eigen::VectorXd &a, double x1, double x2,
                        double x3, double x4, double x5, double x6, double x7,
                        double x8, double x9, double x10, double x11) {
  assert(a.size() == 11);
  return std::abs(a(0) * x1 + a(1) * x2 + a(2) * x3 + a(3) * x4 + a(4) * x5 +
                  a(5) * x6 + a(6) * x7 + a(7) * x8 + a(8) * x9 + a(9) * x10 +
                  a(10) * x11 /*+ a(11)*/); /* + a(12) JIA bias */
}

// inline double predict3_poly2(const Eigen::VectorXd &a, double x1, double x2,
//                              double x3) {
//   assert(a.size() == 3);
//   return  std::abs(a(0) * (x1 + x1 * x1) + a(1) * (x2 + x2 * x2) + a(2) * (x3
//   + x3 * x3));
// }

// inline double computeMean(const Eigen::VectorXd &v) { return v.mean(); }

// inline double computeMedian(Eigen::VectorXd v) {
//   const int n = v.size();
//   assert(n > 0);

//   std::vector<double> data(v.data(), v.data() + n);
//   std::nth_element(data.begin(), data.begin() + n / 2, data.end());

//   if (n % 2 == 1) {
//     return data[n / 2];
//   } else {
//     std::nth_element(data.begin(), data.begin() + n / 2 - 1, data.end());
//     return (data[n / 2] + data[n / 2 - 1]) / 2.0;
//   }
// }

// -----------------------------------------------------------------------------
// 計算 Eigen 向量中的指定百分位數（Percentile）
//
// @param v [in]  欲計算的數值向量（必須為 Eigen::VectorXd）
// @param p [in]  百分位數值，需介於 0.0 到 1.0 之間，例如 0.95 表示第 95 百分位
// @return        回傳對應百分位的值（使用線性內插）
// -----------------------------------------------------------------------------
inline double computePercentile(const Eigen::VectorXd &v, double p) {
  const int n = v.size();
  assert(n > 0 && "向量不能為空");
  assert(p >= 0.0 && p <= 1.0 && "百分位數 p 必須介於 0.0 與 1.0 之間");

  // 將 Eigen 向量轉為 std::vector 以便排序
  std::vector<double> data(v.data(), v.data() + n);
  std::sort(data.begin(), data.end());  // 遞增排序

  // 根據百分位數計算對應的索引位置（可為浮點數）
  const double rank = p * (n - 1);
  const size_t lower = static_cast<size_t>(std::floor(rank));  // 向下取整
  const size_t upper = static_cast<size_t>(std::ceil(rank));   // 向上取整

  // 若 rank 為整數，直接回傳該位置的值
  if (lower == upper) {
    return data[lower];
  } else {
    // 否則進行線性內插（interpolation）
    const double weight = rank - lower;
    return data[lower] * (1.0 - weight) + data[upper] * weight;
  }
}

// -----------------------------------------------------------------------------
// 計算 Eigen 向量的母體標準差（Population Standard Deviation）
//
// @param v     [in] 欲計算的向量資料（Eigen::VectorXd）
// @param mean  [in] 事先計算好的平均值（為避免重複運算而外部傳入）
// @return           回傳標準差（stddev = sqrt(Σ(x - μ)² / n)）
// -----------------------------------------------------------------------------
inline double computeStdDev(const Eigen::VectorXd &v, double mean) {
  const int n = v.size();
  if (n <= 1) return 0.0;  // 單一元素無標準差

  // 將向量中心化後平方，然後加總
  double sum_sq = (v.array() - mean).square().sum();

  // 採用母體標準差（分母為 n，而非 n-1）
  return std::sqrt(sum_sq / n);
}

// -----------------------------------------------------------------------------
// 輸出統計摘要，並回傳 tuple<double, double, double, double, double, double>
// 代表：mean, median, p25, p75, p95, p99
// -----------------------------------------------------------------------------
inline std::tuple<double, double, double, double, double, double>
printStatistics(const std::string &label, const Eigen::VectorXd &data) {
  const double mean = data.mean();
  const double median = computePercentile(data, 0.5);
  const double p25 = computePercentile(data, 0.25);
  const double p75 = computePercentile(data, 0.75);
  const double p95 = computePercentile(data, 0.95);
  const double p99 = computePercentile(data, 0.99);
  const double stddev = computeStdDev(data, mean);

  // 統一輸出格式
  // std::cout << "|--- " << label << " Mean: " << mean << "\n";
  std::cout << "|--- " << label << " 25th Percentile: " << p25 << "\n";
  std::cout << "|--- " << label << " median Percentile: " << median << "\n";
  std::cout << "|--- " << label << " 75th Percentile: " << p75 << "\n";
  std::cout << "|--- " << label << " 95th Percentile: " << p95 << "\n";
  std::cout << "|--- " << label << " 99th Percentile: " << p99 << "\n";
  std::cout << "|--- " << label << " StdDev: " << stddev << " (dispersity)"
            << "\n";

  // 回傳統計結果
  return {mean, median, p25, p75, p95, p99};
}

inline std::tuple<double, double, double, double, double, double>
printStatistics(const Eigen::VectorXd &data) {
  const double mean = data.mean();
  const double median = computePercentile(data, 0.5);
  const double p25 = computePercentile(data, 0.25);
  const double p75 = computePercentile(data, 0.75);
  const double p95 = computePercentile(data, 0.95);
  const double p99 = computePercentile(data, 0.99);

  // 回傳統計結果
  return {mean, median, p25, p75, p95, p99};
}

inline std::tuple<double, int> get_min_max_time(double predicted_time_pt,
                                                double predicted_time_dbt,
                                                double predicted_time_kset,
                                                double predicted_time_dt,
                                                double predicted_time_mt) {
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
  if (predicted_time_dt < min_val) {
    min_val = predicted_time_dt;
    min_id_predict = 3;
  }
  if (predicted_time_mt < min_val) {
    min_val = predicted_time_mt;
    min_id_predict = 4;
  }
  return {min_val, min_id_predict};
}
#endif  // LINEAR_REGRESSION_MODEL_HPP
