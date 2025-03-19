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

#ifndef __READ_H_
#define __READ_H_
#include "data_structure.hpp"

using namespace std;

int read_rules(const char* file_name, vector<Rule>& list) {
  FILE* fp = nullptr;
  fp = fopen(file_name, "r");
  if (fp == nullptr) {
    fprintf(stderr, "error - can not open rules file\n");
    return 0;
  }
  unsigned int sIp[5];
  unsigned int dIp[5];
  unsigned int sPort[2];
  unsigned int dPort[2];
  unsigned int protocol[2];
  int i = 0;
  while (fscanf(fp,
                "@%u.%u.%u.%u/%u\t%u.%u.%u.%u/%u\t%u : %u\t%u : "
                "%u\t%x/%x\t%*x/%*x\t\n",
                &sIp[0], &sIp[1], &sIp[2], &sIp[3], &sIp[4], &dIp[0], &dIp[1],
                &dIp[2], &dIp[3], &dIp[4], &sPort[0], &sPort[1], &dPort[0],
                &dPort[1], &protocol[1], &protocol[0]) != EOF) {
    Rule r;
    r.pri = i;
    r.protocol[0] = (unsigned char)protocol[0];  // mask
    r.protocol[1] = (unsigned char)protocol[1];  // protocol
    r.source_mask = (unsigned char)sIp[4];
    r.destination_mask = (unsigned char)dIp[4];
    int k = 4;
    for (int j = 0; j < 4; j++) {
      r.source_ip[j] = (unsigned char)sIp[--k];
      r.destination_ip[j] = (unsigned char)dIp[k];
    }
    r.source_port[0] = (unsigned short)sPort[0];
    r.source_port[1] = (unsigned short)sPort[1];
    r.destination_port[0] = (unsigned short)dPort[0];
    r.destination_port[1] = (unsigned short)dPort[1];
    list.emplace_back(r);
    ++i;
  }
  fclose(fp);
  return 1;
}

int read_packets(const char* file_name, vector<Packet>& list,
                 vector<int>& check_list) {
  FILE* fp = nullptr;
  fp = fopen(file_name, "r");
  if (fp == nullptr) {
    fprintf(stderr, "error - can not open trace file\n");
    return 0;
  }
  Packet p = {0};
  unsigned int ip_src, ip_des;
  int result;
  while (fscanf(fp, "%u\t%u\t%hu\t%hu\t%u\t%*u\t%d\n", &ip_src, &ip_des,
                &p.source_port, &p.destination_port, &p.protocol,
                &result) != EOF) {
    memcpy(p.source_ip, &ip_src, 4);
    memcpy(p.destination_ip, &ip_des, 4);
    check_list.emplace_back(result);
    list.emplace_back(p);
  }
  fclose(fp);
  return 1;
}

#endif  // !__READ_H_
