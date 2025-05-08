/*
 *	MIT License
 *
 *	Copyright(c) 2022 ShangHai Jiao Tong Univiersity CIT Laboratory.
 *
 *	Permission is hereby granted, free of charge, to any person obtaining a
 *copy of this softwareand associated documentation files(the "Software"), to
 *deal in the Software without restriction, including without limitation the
 *rights to use, copy, modify, merge, publish, distribute, sublicense, and /or
 *sell copies of the Software, and to permit persons to whom the Software is
 *	furnished to do so, subject to the following conditions :
 *
 *	The above copyright noticeand this permission notice shall be included
 *in all copies or substantial portions of the Software.
 *
 *	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 *OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL
 *THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 *IN THE SOFTWARE.
 */

#ifndef __PT_DATA_STRUCTURE_HPP__
#define __PT_DATA_STRUCTURE_HPP__

#include <algorithm>
#include <cstring>
#include <iostream>
#include <vector>
namespace PT {
union MASK {
  uint64_t i_64;
  struct {
    uint32_t smask;
    uint32_t dmask;
  } i_32;
  struct {
    uint8_t mask[8];
  } i_8;
};

union IP {
  uint64_t i_64;
  struct {
    uint32_t sip;
    uint32_t dip;
  } i_32;
  struct {
    uint8_t ip[8];
  } i_8;
};

typedef struct PT_Rule {
  int pri;                    // priority
  unsigned char protocol[2];  // [0] : mask [1] : protocol
  unsigned char source_mask;
  unsigned char destination_mask;
  unsigned char source_ip[4];
  unsigned char destination_ip[4];
  /* JIA  unsigned short */ uint16_t source_port[2];
  /* JIA  unsigned short */ uint16_t destination_port[2];
} PT_Rule;

typedef struct PT_Packet {
  unsigned int protocol;
  unsigned char source_ip[4];
  unsigned char destination_ip[4];
  /* JIA  unsigned short */ uint16_t source_port;
  /* JIA  unsigned short */ uint16_t destination_port;
  // 將 source_ip 與 destination_ip 合併成 uint64_t
  uint64_t toIP64() const {
    uint64_t result = 0;
    // 將前4個 byte 塞入高位（source_ip）
    for (int i = 0; i < 4; ++i) {
      result |= static_cast<uint64_t>(source_ip[i]) << (56 - i * 8);
    }
    // 將後4個 byte 塞入低位（destination_ip）
    for (int i = 0; i < 4; ++i) {
      result |= static_cast<uint64_t>(destination_ip[i]) << (24 - i * 8);
    }
    return result;
  }
} PT_Packet;

struct PT_CacuRule {
  uint32_t pri;  // priority
  IP total_fetch_byte;
  MASK total_mask;
  uint8_t cur_byte;
  uint8_t cur_mask;
  MASK mask;
  IP ip;
  bool is_first;
  uint32_t size;
  uint32_t tSize;

  int acc_inner;
  int acc_leaf;
  int acc_rule;

  PT_CacuRule()
      : total_fetch_byte({0}),
        total_mask({0}),
        mask({0}),
        is_first(false),
        size(1),
        acc_inner(1),
        acc_leaf(0),
        acc_rule(0) {}
};
}  // namespace PT
#endif  //__DATA_STRUCTURE_H_
