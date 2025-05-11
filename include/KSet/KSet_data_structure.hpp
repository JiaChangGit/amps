#ifndef __DATA_STRUCTURE_HPP__
#define __DATA_STRUCTURE_HPP__

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>
#define LowDim 0
#define HighDim 1
#define MAXDIMENSIONS 5

using Packet = std::array<uint32_t, 6>;

struct Rule_KSet {
  Rule_KSet(unsigned int dim = 5)
      : dim(dim),
        pri(0),
        priority(0),
        range(dim, {{0, 0}}),
        prefix_length(dim, 0) {};

  unsigned int dim;
  int pri, priority;
  std::vector<std::array<uint32_t, 2>> range;
  std::vector<uint32_t> prefix_length;

  bool inline isMatch(const Packet& p) const {
    for (size_t i = 0; i < dim; ++i) {
      if (p[i] < range[i][LowDim] || p[i] > range[i][HighDim]) return false;
    }
    return true;
  }
};

#endif
