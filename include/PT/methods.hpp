#ifndef _METHODS_H
#define _MRTHODS_H

#include <random>

#include "pt_tree.hpp"
using namespace std;

void single_thread(vector<uint8_t> set_field, int set_port, bool enable_log,
                   int log_level, bool enable_update, vector<Rule>& rules,
                   vector<Packet>& packets, vector<int>& check_list) {
  struct timespec t1, t2;
  PTtree tree(set_field, set_port);

  /***********************************************************************************************************************/
  // insert
  /***********************************************************************************************************************/
  cout << "\nStart build for single thread...\n|- Using fields:     ";
  for (unsigned int x : set_field) cout << x << ",";
  cout << set_port << endl;
  clock_gettime(CLOCK_REALTIME, &t1);
  for (auto&& r : rules) {
    tree.insert(r);
  }
  clock_gettime(CLOCK_REALTIME, &t2);
  double build_time = get_milli_time(&t1, &t2);
  cout << "|- Construct time:   " << build_time << "ms\n";
  cout << tree.totalNodes << endl;

  cout << "|- Memory footprint: " << (double)tree.mem() / 1024.0 / 1024.0
       << "MB\n";

  /***********************************************************************************************************************/
  // warm up
  /***********************************************************************************************************************/
  for (int i = 0; i < 10; ++i) {
    for (int j = 0; j < 1000; ++j) {
      tree.search(packets[j]);
    }
  }

  /***********************************************************************************************************************/
  // Search
  /***********************************************************************************************************************/
  cout << "\nstart search...\n";
  int res = 0;
  FILE* res_fp = nullptr;
  res_fp = fopen("results.txt", "w");
  double search_time = 0;
  for (int i = 0; i < packets.size(); ++i) {
    clock_gettime(CLOCK_REALTIME, &t1);
    res = tree.search(packets[i]);
    clock_gettime(CLOCK_REALTIME, &t2);
    double _time = get_nano_time(&t1, &t2);
    search_time += _time;

    fprintf(res_fp, "Packet %d \t Result %d \t Time(ns) %f\n", i, res,
            _time / 1.0);
  }
  fclose(res_fp);
  cout << "|- Average search time: " << search_time / packets.size() / 1000.0
       << "um\n";
  cout << "|- Throughput         : " << packets.size() * 1000.0 / search_time
       << "M/s\n";

  /***********************************************************************************************************************/
  // Print Log
  /***********************************************************************************************************************/
  if (enable_log) {
    cout << "\nPrint Log...\n";
    // print node information
    tree.print_node_info(log_level, rules.size());
    // print search information
    FILE* log_fp = nullptr;
    log_fp = fopen("search_info.txt", "w");
    fprintf(log_fp,
            "Search Log [PACKET_ID ACC_INNERNODE ACC_LEAFNODE ACC_TABLE "
            "ACC_RULE ACC_IPNODE ACC_PORTNODE]\n\n");

    double acc_inner, acc_leaf, acc_table, acc_rule;
    acc_inner = acc_leaf = acc_table = acc_rule = 0;
    for (int i = 0; i < packets.size(); ++i) {
      ACL_LOG log;
      tree.search_with_log(packets[i], log);
      acc_inner += log.innerNodes;
      acc_leaf += log.leafNodes;
      acc_table += log.tables;
      acc_rule += log.rules;
      if (log_level > 2)
        fprintf(log_fp, "%d\t%u\t%u\t%u\t%u\t%u\t%u\n", i, log.innerNodes,
                log.leafNodes, log.tables, log.rules, log.ipNodeList.size(),
                log.portNodeList.size());
    }
    cout << "|- Access innerNode avg num: " << acc_inner / packets.size()
         << endl;
    cout << "|- Access leafNode avg num:  " << acc_leaf / packets.size()
         << endl;
    cout << "|- Access table avg num:     " << acc_table / packets.size()
         << endl;
    cout << "|- Access rule avg num:      " << acc_rule / packets.size()
         << endl;
    if (log_level > 2) {
      cout << "|- Write search infomation to search_info.txt...\n";
      fclose(log_fp);
    }
  }

  /***********************************************************************************************************************/
  // update
  /***********************************************************************************************************************/
  if (enable_update) {
    int update_num = 5000;
    cout << "\nStart update...\n";
    bool _u = tree.update(rules, update_num, t1, t2);
  }
}

#endif  // _METHODS_H
