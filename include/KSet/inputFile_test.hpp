/**
 * @file inputFile_test.hpp
 * @brief
 * @author jiachang (jiachanggit@gmail.com)
 * @version 1.0
 * @date 2025-02-20
 *
 * @copyright Copyright (c) 2025  JIA-CHANG
 *
 * @par dialog:
 * <table>
 * <tr><th>Date       <th>Version <th>Author  <th>Description
 * <tr><td>2025-02-20 <td>1.0     <td>jiachang     <td>test loading rule-set and
 * loading trace-set
 * </table>
 */

#ifndef __IO_INPUTFILE_TEST_HPP__
#define __IO_INPUTFILE_TEST_HPP__

#include <fstream>

#include "KSet_data_structure.hpp"

class InputFile_test {
 public:
  void loadRule_test(const std::vector<Rule_KSet>&, const char*);
  void loadPacket_test(const std::vector<Packet>&, const char*);
};

// ------------------- 實作區 -------------------

void InputFile_test::loadRule_test(const std::vector<Rule_KSet>& ruleV,
                                   const char* fileName) {
  std::ofstream outFile(fileName);
  if (!outFile.is_open()) {
    std::cerr << "Failed to open " << fileName << " for writing"
              << "\n";
    return;
  }

  for (const auto& rule : ruleV) {
    outFile << "priority: " << rule.priority << "\n";
    outFile << "prefix_length: ";
    for (const auto& length : rule.prefix_length) {
      outFile << length << " ";
    }
    outFile << "\n";
    outFile << "range: ";
    for (const auto& r : rule.range) {
      outFile << "[" << r[0] << ", " << r[1] << "] ";
    }
    outFile << "\n\n";
  }

  outFile.close();
}

void InputFile_test::loadPacket_test(const std::vector<Packet>& packetV,
                                     const char* fileName) {
  std::ofstream outFile(fileName);
  if (!outFile.is_open()) {
    std::cerr << "Failed to open " << fileName << " for writing"
              << "\n";
    return;
  }
  outFile << "src ip: \tdst ip: \tsrc port: \tdst port: \tprotocol: \match:\n";
  for (const auto& packet : packetV) {
    for (const auto& val : packet) {
      outFile << val << " ";
    }
    outFile << "\n";
  }

  outFile.close();
}

#endif
