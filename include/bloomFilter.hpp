

#pragma once

#include <immintrin.h>  // 包含所有 AVX 指令
#include <x86intrin.h>  // 包含所有 x86 指令，包括 BMI2 和 PEXT

#include <array>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <functional>
#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

#include "DBT_data_structure.hpp"
#include "KSet_data_structure.hpp"

// -----------------------------------------------------------------------------
// BloomFilter（支援 PEXT + AVX2 加速）
// -----------------------------------------------------------------------------
template <size_t BitSize = 8192, size_t HashCount = 4>
class BloomFilter {
  static_assert(BitSize % 64 == 0, "BitSize 必須為 64 的倍數");

  static constexpr size_t kWordCount = BitSize / 64;
  alignas(32) std::array<uint64_t, kWordCount> bit_array;

 public:
  BloomFilter() { clear(); }

  void clear() { std::memset(bit_array.data(), 0, sizeof(bit_array)); }

  template <typename T>
  void insert(const T& item) {
    auto hashes = get_hashes(item);
    for (auto h : hashes) {
      size_t idx = h % BitSize;
      bit_array[idx / 64] |= (1ULL << (idx % 64));
    }
  }

  // AVX2 加速版 contains（一次檢查 4 個 hash）
  template <typename T>
  bool contains(const T& item) const {
    auto hashes = get_hashes(item);
#if defined(__AVX2__)
    for (size_t i = 0; i < HashCount; ++i) {
      size_t idx = hashes[i] % BitSize;
      if ((bit_array[idx / 64] & (1ULL << (idx % 64))) == 0) return false;
    }
    return true;
#else
    for (auto h : hashes) {
      size_t idx = h % BitSize;
      if ((bit_array[idx / 64] & (1ULL << (idx % 64))) == 0) return false;
    }
    return true;
#endif
  }

  template <typename T>
  std::array<uint64_t, HashCount> debug_hashes(const T& item) const {
    return get_hashes(item);
  }

  uint64_t debug_word(size_t word_index) const {
    if (word_index >= kWordCount) return 0;
    return bit_array[word_index];
  }

  size_t bit_count() const {
    size_t count = 0;
    for (auto word : bit_array) count += __builtin_popcountll(word);
    return count;
  }

  void dump_bit_array(const std::string& filename) const {
    std::ofstream out(filename);
    for (size_t i = 0; i < kWordCount; ++i) {
      out << "Word[" << i << "]: 0x" << std::hex << bit_array[i] << " ("
          << std::dec << __builtin_popcountll(bit_array[i]) << " bits set)\n";
    }
    size_t total = bit_count();
    out << "\n  Total bits set: " << total << " / " << BitSize << " ("
        << (100.0 * total / BitSize) << "%)\n";
  }

 private:
  template <typename T>
  std::array<uint64_t, HashCount> get_hashes(const T& item) const {
    std::array<uint64_t, HashCount> hashes{};
    size_t h1 = std::hash<T>{}(item);
    size_t h2 = std::hash<std::string>{}(std::to_string(h1));

    // 準備輸出檔案
    std::ofstream file("hash_debug.txt", std::ios::app);
    file << "h1: " << h1 << "\n";
    file << "h2: " << h2 << "\n";

    for (size_t i = 0; i < HashCount; ++i) {
#ifdef __BMI2__
      hashes[i] = __builtin_ia32_pext_di(h1 + i * h2, BitSize - 1);
#else
      hashes[i] = h1 + i * h2;
#endif
      file << "hashes[" << i << "]: " << hashes[i] << "\n";
    }

    file << "--------------------------\n";
    file.close();
    return hashes;
  }
};

// -----------------------------------------------------------------------------
// Rule - 支援泛型雜湊與自定比較的自訂結構
// -----------------------------------------------------------------------------

namespace std {
template <>
struct hash<Rule> {
  size_t operator()(const Rule& d) const {
    auto murmur = [](uint64_t h) -> uint64_t {
      h ^= h >> 33;
      h *= 0xff51afd7ed558ccdULL;
      h ^= h >> 33;
      h *= 0xc4ceb9fe1a85ec53ULL;
      h ^= h >> 33;
      return h;
    };

    size_t seed = 0;
    // const size_t Dim = (d.dim - 1);
    const size_t Dim = (d.dim);

    for (size_t i = 0; i < Dim; ++i) {
      uint64_t h = (static_cast<uint64_t>(d.range[i][0]) << 32) | d.range[i][1];
      seed ^= murmur(h) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
    return seed;
  }
};
}  // namespace std

namespace DBT {
inline bool operator==(const Rule& a, const Rule& b) {
  return a.protocol.val == b.protocol.val &&
         a.protocol.mask == b.protocol.mask
         /*
                  && a.sip_length == b.sip_length && a.dip_length ==
            b.dip_length
         */
         && a.Port[0][0] == b.Port[0][0] && a.Port[0][1] == b.Port[0][1] &&
         a.Port[1][0] == b.Port[1][0] && a.Port[1][1] == b.Port[1][1] &&
         a.ip.i_64 == b.ip.i_64 && a.mask.i_64 == b.mask.i_64;
};
}  // namespace DBT

namespace std {
template <>
struct hash<DBT::Rule> {
  size_t operator()(const DBT::Rule& r) const {
    auto murmur = [](uint64_t h) -> uint64_t {
      h ^= h >> 33;
      h *= 0xff51afd7ed558ccdULL;
      h ^= h >> 33;
      h *= 0xc4ceb9fe1a85ec53ULL;
      h ^= h >> 33;
      return h;
    };

    size_t seed = 0;

    // 對齊 KSet::Rule 的 range[0]~[4]
    // std::vector<std::array<uint32_t, 2>> dims(4);
    std::vector<std::array<uint32_t, 2>> dims(5);

    // range[0]: SIP
    dims[0][0] = (r.ip.i_32.sip & r.mask.i_32.smask);
    dims[0][1] = dims[0][0] | (~r.mask.i_32.smask & 0xFFFFFFFF);
    // range[1]: DIP
    dims[1][0] = (r.ip.i_32.dip & r.mask.i_32.dmask);
    dims[1][1] = dims[1][0] | (~r.mask.i_32.dmask & 0xFFFFFFFF);
    // range[2]: Src Port
    dims[2][0] = (static_cast<uint32_t>(r.Port[0][0]));
    dims[2][1] = (static_cast<uint32_t>(r.Port[0][1]));
    // range[3]: Dst Port
    dims[3][0] = (static_cast<uint32_t>(r.Port[1][0]));
    dims[3][1] = (static_cast<uint32_t>(r.Port[1][1]));

    //////
    if (0x00 == r.protocol.mask) {
      dims[4][0] = 0;
      dims[4][1] = 0xff;
    } else {
      dims[4][0] = r.protocol.val;
      dims[4][1] = r.protocol.val;
    }
    //////
    // const size_t Dim = (4);
    const size_t Dim = (5);

    for (size_t i = 0; i < Dim; ++i) {
      uint64_t h = (static_cast<uint64_t>(dims[i][0]) << 32) | dims[i][1];
      seed ^= murmur(h) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }

    return seed;
  }
};
}  // namespace std
