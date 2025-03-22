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

#include "data_structure.hpp"

class InputFile_test {
 public:
  void loadRule_test(const std::vector<Rule> &, const char *);
  void loadPacket_test(const std::vector<Packet> &, const char *);
};
#endif
