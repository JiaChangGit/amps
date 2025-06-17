#ifndef _BLOOM_FILTER_HPP_
#define _BLOOM_FILTER_HPP_

// #include <cstdint>
// #include <iostream>
// #include <stdexcept>
#include <random>
#include <vector>

#include "bobHash.hpp"

namespace LONG {
inline namespace V1 {
/**
 * @brief Bloom Filter 實現，支持單執行緒高速查詢，64 位和 128 位數據
 * @tparam T 數據類型，需支持 hash 函數
 */
template <typename T>
class BloomFilter {
 private:
  std::vector<uint8_t> bits_;  // 位陣列（每位用 1 字節存儲，簡化訪問）
  size_t size_;                // 位陣列大小（位數）
  size_t num_hashes_;          // 哈希函數數量
  uint32_t seed_base_;         // 基礎種子值，用於哈希計算

  /**
   * @brief 內部哈希函數，生成單個 64 位哈希值並映射到位陣列索引
   * @param data 輸入數據
   * @param seed 哈希種子
   * @return 位陣列索引
   */
  /* JIA */ __attribute__((always_inline)) inline size_t hash(
      const T& data, uint32_t seed) const {
    // if (sizeof(T) == 0) {
    //   throw std::runtime_error("無效數據大小");
    // }
    uint64_t hash_value = Hash::BOBHash64(
        reinterpret_cast<const uint8_t*>(&data), sizeof(T), seed);
    size_t index = hash_value % size_;
    // if (index >= size_) {
    //   std::cerr << "哈希索引越界: index=" << index << ", size_=" << size_
    //             << ", seed=" << seed << "\n";
    //   throw std::runtime_error("哈希索引越界");
    // }
    return index;
  }

 public:
  /**
   * @brief 構造函數，初始化 Bloom Filter
   * @param expected_items 預期插入的元素數量
   * @param false_positive_rate 期望的誤報率（0 < p < 1）
   */
  BloomFilter(size_t expected_items, double false_positive_rate) {
    // 使用 std::random_device 生成安全種子，限制範圍以避免 prime 數組越界
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint32_t> dist(
        0, MAX_PRIME - 16);  // 保證 seed_base_ + i < MAX_PRIME
    seed_base_ = dist(gen);

    // 驗證輸入參數
    // if (expected_items == 0) {
    //   throw std::invalid_argument("預期元素數量必須大於 0");
    // }
    // if (false_positive_rate <= 0.0 || false_positive_rate >= 1.0) {
    //   throw std::invalid_argument("誤報率必須在 (0, 1) 之間");
    // }

    // 計算位陣列大小：m = -n * ln(p) / (ln(2)^2)
    size_ = static_cast<size_t>(-static_cast<double>(expected_items) *
                                std::log(false_positive_rate) /
                                (std::log(2.0) * std::log(2.0)));
    // 確保大小為 64 的倍數，優化記憶體對齊
    size_ = (size_ + 63) & ~63ULL;
    try {
      bits_.resize((size_ + 7) / 8, 0);  // 每字節存 8 位
    } catch (const std::bad_alloc& e) {
      throw std::runtime_error("位陣列分配失敗: " + std::string(e.what()));
    }

    // 計算哈希函數數量：k = (m/n) * ln(2)
    num_hashes_ = static_cast<size_t>(std::round(
        (static_cast<double>(size_) / expected_items) * std::log(2.0)));
    num_hashes_ = std::max<size_t>(
        1, std::min<size_t>(num_hashes_, 16));  // 限制 k 在合理範圍
  }

  /**
   * @brief 插入元素到 Bloom Filter
   * @param data 要插入的數據
   */
  void insert(const T& data) {
    // 單執行緒逐一計算哈希並設置位
    for (size_t i = 0; i < num_hashes_; ++i) {
      size_t index = hash(data, seed_base_ + i);
      // if (index / 8 >= bits_.size()) {
      //   std::cerr << "位陣列越界: index=" << index << ", byte=" << (index /
      //   8)
      //             << ", bits_.size()=" << bits_.size() << "\n";
      //   throw std::runtime_error("位陣列越界");
      // }
      bits_[index / 8] |= (1 << (index % 8));
    }
  }

  /**
   * @brief 查詢元素是否存在
   * @param data 要查詢的數據
   * @return true 如果元素可能存在，false 如果元素確定不存在
   */
  /* JIA */ __attribute__((always_inline)) inline bool contains(
      const T& data) const {
    // 單執行緒逐一檢查哈希對應的位
    for (size_t i = 0; i < num_hashes_; ++i) {
      size_t index = hash(data, seed_base_ + i);
      // if (index / 8 >= bits_.size()) {
      //   std::cerr << "查詢位陣列越界: index=" << index
      //             << ", byte=" << (index / 8)
      //             << ", bits_.size()=" << bits_.size() << "\n";
      //   throw std::runtime_error("查詢位陣列越界");
      // }
      if (!(bits_[index / 8] & (1 << (index % 8)))) {
        return false;
      }
    }

    return true;
  }

  /**
   * @brief 獲取位陣列中 1 的比例（填充率）
   * @return 填充率（0.0 到 1.0）
   */
  double fill_ratio() const {
    size_t set_bits = 0;
    for (size_t i = 0; i < bits_.size(); ++i) {
      uint8_t byte = bits_[i];
      for (int j = 0; j < 8; ++j) {
        set_bits += (byte >> j) & 1;
      }
    }
    return static_cast<double>(set_bits) / size_;
  }
};
}  // namespace V1
}  // namespace LONG

#endif
