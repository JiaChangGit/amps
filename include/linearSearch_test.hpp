#ifndef _LINEARSEARCH_HPP_
#define _LINEARSEARCH_HPP_

// #include <fstream>

#include "KSet_data_structure.hpp"

class LinearSearch {
 public:
  // explicit LinearSearch() { results.resize(0); };
  // explicit LinearSearch(int _packetNum) { results.resize(_packetNum); };

  int search(const std::vector<Rule_KSet> &rules, const Packet &packet) {
    int counter = 0;

    for (const auto &rule : rules) {
      if (rule.isMatch(packet)) {
        // std::cout << "match: " << rule.priority << "\n";
        break;
      }
      ++counter;
    }
    return rules[counter].priority;
  };

 private:
  // std::vector<int> results;
};
#endif
