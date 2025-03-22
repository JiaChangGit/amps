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

#ifndef __DBT_METHODS_HPP__
#define __DBT_MRTHODS_HPP__
#include <time.h>
#include <unistd.h>

#include <list>
#include <random>

#include "DBT_core.hpp"

using namespace std;
namespace DBT {
void single_thread(bool enable_update, vector<Rule>& rules,
                   vector<Packet>& packets, vector<int>& check_list) {
  Timer timer;
  cout << "\nbuild for single thread...\n";
  cout << "binth=" << BINTH << " th_b=" << END_BOUND << " K=" << TOP_K
       << " th_c=" << C_BOUND << endl
       << endl;

  DBTable dbt(rules, BINTH);
  timer.timeReset();
  dbt.construct();

  cout << "Construction Time: " << timer.elapsed_ns() << " ns\n";

  dbt.mem();

  cout << "\nstart search...\n";
  uint32_t res = 0;
  FILE* res_fp = nullptr;
  res_fp = fopen("./INFO/DBT_results.txt", "w");
  unsigned long long search_time = 0, _search_time = 0;

  const size_t packetNum = packets.size();
  for (int i = 0; i < packetNum; ++i) {
    timer.timeReset();
    res = dbt.search(packets[i]);
    _search_time = timer.elapsed_ns();
    search_time += _search_time;

    fprintf(res_fp, "Packet %d \t Result %d \t Time(ns) %f\n", i, res,
            _search_time / 1.0);
  }
  fclose(res_fp);
  cout << "|- Average search time: " << search_time / packetNum << "ns\n";

  dbt.search_with_log(packets);

  // update
  if (enable_update) {
    int update_num = 5000;
    cout << "\nStart update...\n";
    random_device seed;
    mt19937 rd(seed());
    uniform_int_distribution<> dis(0, rules.size() * 0.6);
    double update_time = 0;

    unsigned long long delete_time = 0, _delete_time = 0;
    unsigned long long insert_time = 0, _insert_time = 0;
    for (int i = 0; i < update_num; ++i) {
      int cur_idx = dis(rd);
      timer.timeReset();
      dbt.remove(rules[cur_idx]);
      _delete_time = timer.elapsed_ns();
      delete_time += _delete_time;

      timer.timeReset();
      dbt.insert(rules[cur_idx]);
      _insert_time = timer.elapsed_ns();
      insert_time += _insert_time;
    }

    cout << "|- Average delete time: " << delete_time / (update_num) << "ns\n";
    cout << "|- Average insert time: " << insert_time / (update_num) << "ns\n";
    cout << "|- Average update time: "
         << (delete_time + insert_time) / (update_num * 2) << "ns\n\n";
  }
}
}  // namespace DBT
#endif  // _METHODS_H
