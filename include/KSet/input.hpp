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

#include <cstdio>
#include <cstdlib>

#include "KSet_data_structure.hpp"

class InputFile {
 public:
  void loadRule(std::vector<Rule> &, const char *);
  void loadPacket(std::vector<Packet> &, const char *);
};

// ------------------- 實作區 -------------------

void InputFile::loadRule(std::vector<Rule> &rule, const char *fileName) {
  FILE *fp = nullptr;
  fp = fopen(fileName, "r");
  if (fp == nullptr) {
    fprintf(stderr, "error - can not open rules file\n");
    exit(1);
  }
  uint32_t tmp;
  uint32_t max_uint = 0xFFFFFFFF;
  uint32_t mask;
  uint32_t sip1, sip2, sip3, sip4, smask;
  uint32_t dip1, dip2, dip3, dip4, dmask;
  uint32_t sport1, sport2;
  uint32_t dport1, dport2;
  uint32_t protocal, protocol_mask;
  size_t number_rule = 0;  // number of rules
  size_t max_pri = 0;
  while (1) {
    Rule r;
    std::array<uint32_t, 2> points;

    if (fscanf(fp,
               "@%u.%u.%u.%u/%u\t%u.%u.%u.%u/%u\t%u : %u\t%u : "
               "%u\t%x/%x\t%*x/%*x\t\n",
               &sip1, &sip2, &sip3, &sip4, &smask, &dip1, &dip2, &dip3, &dip4,
               &dmask, &sport1, &sport2, &dport1, &dport2, &protocal,
               &protocol_mask) == EOF)
      break;

    if (smask == 0) {
      points[0] = 0;
      points[1] = 0xFFFFFFFF;
    } else if (smask > 0 && smask <= 8) {
      tmp = sip1 << 24;
      mask = ~(max_uint >> smask);
      tmp &= mask;
      points[0] = tmp;
      points[1] = points[0] + (1 << (32 - smask)) - 1;
    } else if (smask > 8 && smask <= 16) {
      tmp = sip1 << 24;
      tmp += sip2 << 16;
      mask = ~(max_uint >> smask);
      tmp &= mask;
      points[0] = tmp;
      points[1] = points[0] + (1 << (32 - smask)) - 1;
    } else if (smask > 16 && smask <= 24) {
      tmp = sip1 << 24;
      tmp += sip2 << 16;
      tmp += sip3 << 8;
      mask = ~(max_uint >> smask);
      tmp &= mask;
      points[0] = tmp;
      points[1] = points[0] + (1 << (32 - smask)) - 1;
    } else if (smask > 24 && smask <= 32) {
      tmp = sip1 << 24;
      tmp += sip2 << 16;
      tmp += sip3 << 8;
      tmp += sip4;
      mask = smask != 32 ? ~(max_uint >> smask) : max_uint;
      tmp &= mask;
      points[0] = tmp;
      points[1] = points[0] + (1 << (32 - smask)) - 1;
    } else {
      std::cerr << "Src IP length exceeds 32\n";
      exit(1);
    }
    r.range[0] = points;

    if (dmask == 0) {
      points[0] = 0;
      points[1] = 0xFFFFFFFF;
    } else if (dmask > 0 && dmask <= 8) {
      tmp = dip1 << 24;
      mask = ~(max_uint >> dmask);
      tmp &= mask;
      points[0] = tmp;
      points[1] = points[0] + (1 << (32 - dmask)) - 1;
    } else if (dmask > 8 && dmask <= 16) {
      tmp = dip1 << 24;
      tmp += dip2 << 16;
      mask = ~(max_uint >> dmask);
      tmp &= mask;
      points[0] = tmp;
      points[1] = points[0] + (1 << (32 - dmask)) - 1;
    } else if (dmask > 16 && dmask <= 24) {
      tmp = dip1 << 24;
      tmp += dip2 << 16;
      tmp += dip3 << 8;
      mask = ~(max_uint >> dmask);
      tmp &= mask;
      points[0] = tmp;
      points[1] = points[0] + (1 << (32 - dmask)) - 1;
    } else if (dmask > 24 && dmask <= 32) {
      tmp = dip1 << 24;
      tmp += dip2 << 16;
      tmp += dip3 << 8;
      tmp += dip4;
      mask = dmask != 32 ? ~(max_uint >> dmask) : max_uint;
      tmp &= mask;
      points[0] = tmp;
      points[1] = points[0] + (1 << (32 - dmask)) - 1;
    } else {
      std::cerr << "Dest IP length exceeds 32\n";
      exit(1);
    }
    r.range[1] = points;

    points[0] = sport1;
    points[1] = sport2;
    r.range[2] = points;

    points[0] = dport1;
    points[1] = dport2;
    r.range[3] = points;

    if (protocol_mask == 0xFF) {
      points[0] = protocal;
      points[1] = protocal;
    } else if (protocol_mask == 0) {
      points[0] = 0;
      points[1] = 0xFF;
    } else {
      std::cerr << "Protocol mask error\n";
      exit(1);
    }
    r.range[4] = points;

    r.prefix_length[0] = smask;
    r.prefix_length[1] = dmask;
    if (sport1 == sport2) {
      // 0: portS is exact value
      r.prefix_length[2] = 0;
    } else {
      r.prefix_length[2] = 1;
    }
    if (dport1 == dport2) {
      // 0: portD is exact value
      r.prefix_length[3] = 0;
    } else {
      r.prefix_length[3] = 1;
    }
    if (protocol_mask != 0x00) {
      // 0: protocol is exact value
      r.prefix_length[4] = 0;
    } else {
      r.prefix_length[4] = 1;
    }

    ++number_rule;
    r.pri = number_rule;
    rule.emplace_back(r);
  }
  rule.shrink_to_fit();

  // std::cout<<"the number of rules = "<< number_rule<<"\n";
  max_pri = number_rule - 1;
  for (size_t i = 0; i < number_rule; ++i) {
    rule[i].priority = max_pri - i;
  }
  return;
}

void InputFile::loadPacket(std::vector<Packet> &packets, const char *fileName) {
  FILE *fp = nullptr;
  fp = fopen(fileName, "r");
  if (fp == nullptr) {
    fprintf(stderr, "error - can not open trace file\n");
    exit(1);
  }
  unsigned int header[MAXDIMENSIONS];
  unsigned int number_pkt = 0;  // number of packets
  unsigned int proto_mask, fid;

  while (1) {
    if (fscanf(fp, "%u\t%u\t%u\t%u\t%u\t%u\t%u\n", &header[0], &header[1],
               &header[2], &header[3], &header[4], &proto_mask, &fid) == EOF)
      break;
    Packet p = {header[0], header[1], header[2], header[3], header[4], fid};
    ++number_pkt;
    packets.emplace_back(p);
  }
  packets.shrink_to_fit();
  // std::cout<<"the number of pkts = "<< number_pkt<<"\n";

  return;
}
// <Source Address>	<Destination Address>	<Source Port>	<Destination
// Port>	<Protocol>	<Filter Number>

// The Filter Number simply records the filter used to generate the header.
// This may NOT be the best-matching (or first-matching) filter for the packet
// header.

#endif
