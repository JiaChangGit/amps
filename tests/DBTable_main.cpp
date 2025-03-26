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

#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>

#include "DBT_method.hpp"
#include "DBT_read.hpp"

int DBT::TOP_K = 4;
double DBT::END_BOUND = 0.8;
int DBT::C_BOUND = 32;
int DBT::BINTH = 4;
uint32_t DBT::maskBit[33] = {
    0,          0x80000000, 0xC0000000, 0xE0000000, 0xF0000000, 0xF8000000,
    0xFC000000, 0xFE000000, 0xFF000000, 0xFF800000, 0xFFC00000, 0xFFE00000,
    0xFFF00000, 0xFFF80000, 0xFFFC0000, 0xFFFE0000, 0xFFFF0000, 0xFFFF8000,
    0xFFFFC000, 0xFFFFE000, 0xFFFFF000, 0xFFFFF800, 0xFFFFFC00, 0xFFFFFE00,
    0xFFFFFF00, 0xFFFFFF80, 0xFFFFFFC0, 0xFFFFFFE0, 0xFFFFFFF0, 0xFFFFFFF8,
    0xFFFFFFFC, 0xFFFFFFFE, 0xFFFFFFFF};

uint32_t DBT::getBit[32] = {
    0x80000000, 0x40000000, 0x20000000, 0x10000000, 0x08000000, 0x04000000,
    0x02000000, 0x01000000, 0x00800000, 0x00400000, 0x00200000, 0x00100000,
    0x00080000, 0x00040000, 0x00020000, 0x00010000, 0x00008000, 0x00004000,
    0x00002000, 0x00001000, 0x00000800, 0x00000400, 0x00000200, 0x00000100,
    0x00000080, 0x00000040, 0x00000020, 0x00000010, 0x00000008, 0x00000004,
    0x00000002, 0x00000001};
using namespace std;

int main(int argc, char* argv[]) {
  // process argument
  if (argc == 1) {
    fprintf(stderr, "use -h(--help) to print the usage guideline.\n");
    return 0;
  }
  vector<DBT::Rule> rules;
  vector<DBT::Packet> packets;
  vector<int> check_list;
  Timer timer;

  constexpr bool enable_log = true;
  bool enable_update = false;

  int opt;
  struct option opts[] = {{"ruleset", 1, NULL, 'r'},
                          {"packet", 1, NULL, 'p'},
                          {"update", 0, NULL, 'u'},
                          {"binth", 1, NULL, 'b'},
                          {"th_b", 1, NULL, 'e'},
                          {"th_c", 1, NULL, 'c'},
                          {"top-k", 1, NULL, 'k'},
                          {"help", 0, NULL, 'h'},
                          {0, 0, 0, 0}};

  while ((opt = getopt_long(argc, argv, "r:p:b:e:c:k:t:uh", opts, NULL)) !=
         -1) {
    switch (opt) {
      case 'r':
        cout << "Read ruleset:  " << optarg << endl;
        if (!read_rules(optarg, rules)) return -1;
        break;
      case 'p':
        cout << "Rread packets: " << optarg << endl;
        if (!read_packets(optarg, packets, check_list)) return -1;
        break;
      case 'b':
        DBT::BINTH = atoi(optarg);
        break;
      case 'e':
        DBT::END_BOUND = atof(optarg);
        break;
      case 'c':
        DBT::C_BOUND = atoi(optarg);
        break;
      case 'k':
        DBT::TOP_K = atoi(optarg);
        break;
      case 'u':
        enable_update = true;
        cout << "Enable update\n";
        break;
      case 'h':
        cout << "\n************************************************************"
                "**************************************************************"
                "**********************************\n";
        cout << "* -r(--ruleset): Input the rule set file. This argument must "
                "be specified. (Example: [-r acl1])                            "
                "                                *\n";
        cout << "* -p(--packet):  Input the packet set file. If not set, the "
                "program will generate randomly. (Example: [-p acl1_trace])    "
                "                                 *\n";
        cout << "* -u(--update):  Enable update. (Example: [-u])               "
                "                                                              "
                "                               *\n";
        cout << "* -h(--help):    Print the usage guideline.                   "
                "                                                              "
                "                               *\n";
        cout << "**************************************************************"
                "**************************************************************"
                "********************************\n\n";
        if (argc == 2) return 0;
        break;
      case '?':
        fprintf(stderr, "error-unknown argument -%c.", optopt);
        return -1;
      default:
        break;
    }
  }

  single_thread(enable_update, rules, packets);

  cout << "\nprogram complete\n\n";
  return 0;
}
