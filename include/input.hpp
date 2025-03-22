/**
 * @file input.hpp
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
 * <tr><td>2025-02-20 <td>1.0     <td>jiachang     <td>load rule-set and
 * trace-set
 * </table>
 */

#ifndef __IO_INPUT_HPP__
#define __IO_INPUT_HPP__

#include "data_structure.hpp"

class InputFile {
 public:
  void loadRule(std::vector<Rule> &, const char *);
  void loadPacket(std::vector<Packet> &, const char *);
};
#endif
