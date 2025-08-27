#ifndef _KNNCLASSIFIER_HPP_
#define _KNNCLASSIFIER_HPP_
// #include <algorithm>
// #include <array>
#include <cstdint>
// #include <vector>
#include <map>

#include "nanoflann.hpp"
// using Packet = std::array<uint32_t, 6>;
//  --------------------------------------------
//  枚舉：用於標記對應的資料結構類型（5種類型）
//  --------------------------------------------
enum class DataStructure : uint8_t {
  PTTree = 0,
  DBTable,
  KSet,
  DTuple,
  MTuple
};

// --------------------------------------------
// 資料樣本：包含一筆 6 維輸入資料與對應的分類標籤
// - 其中只有前 5 維會被用於 KNN 查詢
// - 第 6 維無意義
// --------------------------------------------
struct LabeledSample {
  Packet data;
  DataStructure label;
};

// --------------------------------------------
// nanoflann 專用資料集接頭（Adaptor）
// - 負責提供 KD-Tree 所需的資料讀取介面
// - 不擁有資料，只是參考外部傳入的 vector
// --------------------------------------------
// struct DatasetAdaptor {
//   const std::vector<LabeledSample>& samples;

//   // 回傳資料點總數（KD-Tree 所需）
//   inline size_t kdtree_get_point_count() const { return samples.size(); }

//   // 回傳第 idx 筆資料在第 dim 維的值（僅前5維有效）
//   inline double kdtree_get_pt(const size_t idx, const size_t dim) const {
//     return static_cast<double>(samples[idx].data[dim]);
//   }

//   // 若不支援 bounding box，可以直接回傳 false（不影響查詢）
//   template <class BBOX>
//   bool kdtree_get_bbox(BBOX&) const {
//     return false;
//   }
// };
struct DatasetAdaptor {
  const std::vector<LabeledSample>& samples;
  std::array<std::pair<float, float>, 5> bbox;  // 緩存邊界框

  DatasetAdaptor(const std::vector<LabeledSample>& s) : samples(s) {
    // 預計算邊界框
    for (size_t dim = 0; dim < 5; ++dim) {
      float min_val = std::numeric_limits<float>::max();
      float max_val = std::numeric_limits<float>::lowest();
      for (const auto& sample : samples) {
        float val = static_cast<float>(sample.data[dim]);
        min_val = std::min(min_val, val);
        max_val = std::max(max_val, val);
      }
      bbox[dim] = {min_val, max_val};
    }
  }

  inline size_t kdtree_get_point_count() const { return samples.size(); }

  inline float kdtree_get_pt(const size_t idx, const size_t dim) const {
    return static_cast<float>(samples[idx].data[dim]);
  }

  template <class BBOX>
  bool kdtree_get_bbox(BBOX& bb) const {
    for (size_t i = 0; i < 5; ++i) {
      bb[i].low = bbox[i].first;
      bb[i].high = bbox[i].second;
    }
    return true;
  }
};

// --------------------------------------------
// KNNClassifier：基於 KD-Tree 的近鄰分類器（Header-only）
// - 採用 nanoflann 建構 KD-Tree，支援快速查詢
// - 查詢時使用前 5 維進行相似度評估
// --------------------------------------------
class KNNClassifier {
  // 定義 KD-Tree 類型：使用 L2 距離、5 維空間
  using KDTree = nanoflann::KDTreeSingleIndexAdaptor<
      // nanoflann::L2_Simple_Adaptor<double, DatasetAdaptor>,  // 距離函式（L2
      // Norm）
      nanoflann::L1_Adaptor<double, DatasetAdaptor>,  // 改用 L1 距離
      DatasetAdaptor,                                 // 資料來源
      5>;                                             // 使用前五維（維度）

  const std::vector<LabeledSample>& samples;  // 原始資料（外部傳入）
  DatasetAdaptor adaptor;                     // nanoflann 接頭（包裝用）
  KDTree index;                               // nanoflann 的 KD-Tree 查詢器

 public:
  // 建構子：建構 KD-Tree
  // - 使用 adaptor 將資料傳給 nanoflann
  // - 預設每個 leaf node 容納 50 筆資料
  explicit KNNClassifier(const std::vector<LabeledSample>& s)
      : samples(s),
        adaptor{samples},
        index(5, adaptor, nanoflann::KDTreeSingleIndexAdaptorParams(50)) {
    index.buildIndex();  // 實際建立 KD-Tree 索引
  }
  /// 返回 nanoflann KD-tree index 使用的位元組數 (bytes)
  size_t indexMemoryBytes() {
    // 注意：KDTreeBaseClass::usedMemory 要傳入 non-const Derived&；
    // 在這個非 const 成員函式中直接呼叫即可。
    return index.usedMemory(index);
  }

  /// 方便的 MB 顯示
  double indexMemoryMB() {
    return static_cast<double>(indexMemoryBytes()) / 1024.0 / 1024.0;
  }

  // 預測函式：給定一筆 6 維輸入資料，使用 KNN 找到最近鄰投票
  // - 預設取 K = 3 個鄰居
  __attribute__((always_inline)) inline DataStructure predict_vote(
      const std::array<double, 5>& query, size_t K = 3) const {
    // 儲存 K 個最近鄰的索引與距離
    std::vector<size_t> ret_indexes(K);
    std::vector<double> out_dists(K);

    // 建立查詢結果容器
    nanoflann::KNNResultSet<double> resultSet(K);
    resultSet.init(ret_indexes.data(), out_dists.data());

    // 執行 KNN 查詢，設置 eps=0.5 or NULL
    index.findNeighbors(resultSet, query.data(), nanoflann::SearchParameters());

    // 統計各分類的投票數
    std::map<DataStructure, int> vote_count;
    for (size_t i = 0; i < resultSet.size(); ++i)
      vote_count[samples[ret_indexes[i]].label]++;

    // 回傳票數最多的分類（若平手，取 first 出現者）
    return std::max_element(
               vote_count.begin(), vote_count.end(),
               [](const auto& a, const auto& b) { return a.second < b.second; })
        ->first;
  }

  // - 預設取 K = 1 個鄰居
  __attribute__((always_inline)) inline DataStructure predict_one(
      const std::array<double, 5>& query) const {
    size_t nearest_index = 0;
    double nearest_dist = 0.0f;

    nanoflann::KNNResultSet<double> resultSet(1);
    resultSet.init(&nearest_index, &nearest_dist);
    // 設置 eps=0.5 or NULL
    index.findNeighbors(resultSet, query.data(), nanoflann::SearchParameters());

    return samples[nearest_index].label;
  }
};
#endif
