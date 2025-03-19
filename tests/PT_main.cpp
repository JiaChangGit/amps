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

#include <random>

#include "methods.hpp"
#include "pt_tree.hpp"
#include "read.hpp"

using namespace std;

int main(int argc, char* argv[]) {
  if (argc == 1) {
    fprintf(stderr, "use -h(--help) to print the usage guideline.\n");
    return 0;
  }
  vector<Rule> rules;
  vector<Packet> packets;
  vector<int> check_list;

  struct timespec t1, t2;
  constexpr bool enable_log = true;
  constexpr int log_level = 3;  // {1,2,3}
  bool enable_update = false;
  vector<uint8_t> set_field;
  int set_port = 1;

  int opt;
  struct option opts[] = {{"ruleset", 1, NULL, 'r'},
                          {"packet", 1, NULL, 'p'},
                          {"fields", 1, NULL, 'f'},
                          {"log", 1, NULL, 'l'},
                          {"update", 0, NULL, 'u'},
                          {"help", 0, NULL, 'h'},
                          {0, 0, 0, 0}};

  while ((opt = getopt_long(argc, argv, "r:p:f:uh", opts, NULL)) != -1) {
    switch (opt) {
      case 'r':
        cout << "Read ruleset:  " << optarg << endl;
        if (!read_rules(optarg, rules)) return -1;
        break;
      case 'p':
        cout << "Rread packets: " << optarg << endl;
        if (!read_packets(optarg, packets, check_list)) return -1;
        break;
      case 'f': {
        vector<int> tmp_in_field;
        int i = 0;
        while (optarg[i] != '\0') {
          if (optarg[i] != ',') {
            char c = optarg[i];
            tmp_in_field.emplace_back(atoi(&c));
          }
          ++i;
        }
        cout << "Set pTree field: ";
        for (i = 0; i < tmp_in_field.size() - 1; ++i) {
          cout << tmp_in_field[i] << " ";
          set_field.emplace_back(tmp_in_field[i]);
        }
        cout << "\nSet aTree port field: " << tmp_in_field[i] << endl;
        set_port = tmp_in_field[i];
        break;
      }
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
        cout << "* -f(--fields):  Set the pTree and aTree used fields, using "
                "\',\' to separation. The last on is the port setting, 0 is "
                "source port, 1 is destination port.   *\n";
        cout << "*                Using 0-3 to express source ip 1-4 byte and "
                "4-7 to express destination ip 1-4 byte. (Example: [-f "
                "4,0,1,1])                               *\n";
        cout << "* -l(--log):     Enable the log. Have three level 1-3. "
                "(Example: [-l 3])                                             "
                "                                      *\n";
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

  setmaskHash();

  /***********************************************************************************************************************/
  // search config
  /***********************************************************************************************************************/
  if (set_field.size() == 0) {
    clock_gettime(CLOCK_REALTIME, &t1);
    CacuInfo cacu(rules);
    cacu.read_fields();
    set_field = cacu.cacu_best_fields();
    set_port = 1;
    clock_gettime(CLOCK_REALTIME, &t2);
    double search_config_time = get_milli_time(&t1, &t2);
    cout << "search config time: " << (search_config_time / 1000.0) << " s"
         << endl;
  }
  for (int i = 0; i < set_field.size(); ++i) cout << set_field[i] << " ";

  single_thread(set_field, set_port, enable_log, log_level, enable_update,
                rules, packets, check_list);

  cout << "\nProgram complete.\n\n";
  return 0;
}
