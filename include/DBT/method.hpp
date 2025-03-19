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

#ifndef __METHODS_HPP__
#define __MRTHODS_HPP__
#include <time.h>
#include <unistd.h>

#include <list>
#include <random>

#include "core.hpp"

using namespace std;

void single_thread(bool enable_update, vector<Rule>& rules,
                   vector<Packet>& packets, vector<int>& check_list) {
  struct timespec t1, t2;
  cout << "\nbuild for single thread...\n";
  cout << "binth=" << BINTH << " th_b=" << END_BOUND << " K=" << TOP_K
       << " th_c=" << C_BOUND << endl
       << endl;

  DBTable dbt(rules, BINTH);
  clock_gettime(CLOCK_REALTIME, &t1);
  dbt.construct();
  clock_gettime(CLOCK_REALTIME, &t2);
  cout << "Construction Time: " << get_milli_time(&t1, &t2) << " ms\n";

  dbt.mem();

  cout << "\nstart search...\n";
  uint32_t res = 0;
  FILE* res_fp = nullptr;
  res_fp = fopen("results.txt", "w");
  double search_time = 0;
  clock_gettime(CLOCK_REALTIME, &t1);
  for (int i = 0; i < packets.size(); ++i) {
    clock_gettime(CLOCK_REALTIME, &t1);
    res = dbt.search(packets[i]);
    clock_gettime(CLOCK_REALTIME, &t2);
    double _time = get_nano_time(&t1, &t2);
    search_time += _time;

    fprintf(res_fp, "Packet %d \t Result %d \t Time(ns) %f\n", i, res,
            _time / 1.0);
  }
  fclose(res_fp);
  cout << "|- Average search time: " << search_time / packets.size() / 1000.0
       << "us\n";
  cout << "|- Throughput         : " << packets.size() * 1000.0 / search_time
       << "M/s\n";

  dbt.search_with_log(packets);

  // update
  int update_num = 5000;
  cout << "\nStart update...\n";
  random_device seed;
  mt19937 rd(seed());
  uniform_int_distribution<> dis(0, rules.size() * 0.6);
  double update_time = 0;

  for (int i = 0; i < update_num; ++i) {
    int cur_idx = dis(rd);
    clock_gettime(CLOCK_REALTIME, &t1);
    dbt.remove(rules[cur_idx]);
    dbt.insert(rules[cur_idx]);
    clock_gettime(CLOCK_REALTIME, &t2);
    update_time += get_nano_time(&t1, &t2);
  }
  cout << "|- Average update time: " << update_time / (update_num * 2) / 1000.0
       << "us\n\n";
}

#endif  // _METHODS_H
