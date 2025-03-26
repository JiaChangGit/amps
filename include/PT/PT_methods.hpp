#ifndef __PT_METHODS_HPP__
#define __PT_METHODS_HPP__

#include <random>

#include "pt_tree.hpp"
using namespace std;
namespace PT {
void single_thread(vector<uint8_t> set_field, int set_port, bool enable_update,
                   vector<PT_Rule>& PT_rule, vector<PT_Packet>& PT_packets) {
  Timer timer;
  PTtree tree(set_field, set_port);
  const size_t packetNum = PT_packets.size();

  /***********************************************************************************************************************/
  // insert
  /***********************************************************************************************************************/
  cout << "\nStart build for single thread...\n|- Using fields:     ";
  for (unsigned int x : set_field) cout << x << ",";
  cout << set_port << endl;
  timer.timeReset();
  for (auto&& r : PT_rule) {
    tree.insert(r);
  }
  cout << "|- Construct time:   " << timer.elapsed_ns() << "ns\n";
  cout << "|- tree.totalNodes: " << tree.totalNodes << endl;

  cout << "|- Memory footprint: " << (double)tree.mem() / 1024.0 / 1024.0
       << "MB\n";

  /***********************************************************************************************************************/
  // warm up
  /***********************************************************************************************************************/
  for (int i = 0; i < 10; ++i) {
    for (int j = 0; j < 1000; ++j) {
      tree.search(PT_packets[j]);
    }
  }

  /***********************************************************************************************************************/
  // Search
  /***********************************************************************************************************************/
  cout << "\nstart search...\n";
  int res = 0;
  FILE* res_fp = nullptr;
  res_fp = fopen("./INFO/PT_results.txt", "w");
  unsigned long long search_time = 0, _search_time = 0;
  for (int i = 0; i < packetNum; ++i) {
    timer.timeReset();
    res = tree.search(PT_packets[i]);
    _search_time = timer.elapsed_ns();
    search_time += _search_time;

    fprintf(res_fp, "Packet %d \t Result %d \t Time(ns) %f\n", i, res,
            _search_time / 1.0);
  }
  fclose(res_fp);
  cout << "|- Average search time: " << search_time / packetNum << "ns\n";

  /***********************************************************************************************************************/
  // Print Log
  /***********************************************************************************************************************/

  cout << "\nPrint Log...\n";
  // print node information
  tree.print_node_info(PT_rule.size());
  // print search information
  FILE* log_fp = nullptr;
  log_fp = fopen("./INFO/PT_search_info.txt", "w");
  fprintf(log_fp,
          "Search Log [PACKET_ID ACC_INNERNODE ACC_LEAFNODE ACC_TABLE "
          "ACC_RULE ACC_IPNODE ACC_PORTNODE]\n\n");

  double acc_inner, acc_leaf, acc_table, acc_rule;
  acc_inner = acc_leaf = acc_table = acc_rule = 0;
  for (int i = 0; i < packetNum; ++i) {
    ACL_LOG log;
    tree.search_with_log(PT_packets[i], log);
    acc_inner += log.innerNodes;
    acc_leaf += log.leafNodes;
    acc_table += log.tables;
    acc_rule += log.rules;
    fprintf(log_fp, "%d\t%u\t%u\t%u\t%u\t%u\t%u\n", i, log.innerNodes,
            log.leafNodes, log.tables, log.rules, log.ipNodeList.size(),
            log.portNodeList.size());
  }
  cout << "|- Access innerNode avg num: " << acc_inner / packetNum << endl;
  cout << "|- Access leafNode avg num:  " << acc_leaf / packetNum << endl;
  cout << "|- Access table avg num:     " << acc_table / packetNum << endl;
  cout << "|- Access rule avg num:      " << acc_rule / packetNum << endl;
  cout << "|- Write search infomation to search_info.txt...\n";
  fclose(log_fp);

  /***********************************************************************************************************************/
  // update
  /***********************************************************************************************************************/
  if (enable_update) {
    int update_num = 5000;
    cout << "\nStart update...\n";
    bool _u = tree.update(PT_rule, update_num, timer);
  }
}
}  // namespace PT
#endif  // _METHODS_H
