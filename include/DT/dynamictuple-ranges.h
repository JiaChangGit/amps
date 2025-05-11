#ifndef DYNAMICTUPLERANGES_H
#define DYNAMICTUPLERANGES_H

#include <cmath>

#include "elementary_DT_MT.h"
#include "io_DT_MT.h"

#define SPLIT_SELF 1
#define SPLIT_SRC 2
#define SPLIT_DST 3
#define SPLIT_IGNORE 4

// To define TUPLEINFO if you want to know each access number
// DynamicTuple_Demo need to define TUPLEINFO
// DynamicTuple_Basic and DynamicTuple can be used without TUPLEINFO, which will
// malloc more memory
#define TUPLEINFO

using namespace std;

struct TupleRange {
  int x1, y1, x2, y2;
  TupleRange(int _x1, int _y1, int _x2, int _y2) {
    x1 = _x1;
    y1 = _y1;
    x2 = _x2;
    y2 = _y2;
  }
  TupleRange() {}
};

struct DtRule {
  uint64_t src_dst_ip;
  uint64_t reduced_src_dst_ip;
  int priority;
  int check_num;
  char src_prefix_len, dst_prefix_len;
  bool first;
};

struct TupleCost {
  uint64_t cost;
  char split_type;
  char split_prefix_len;
};

struct TupleInfo {
  uint64_t check_tuple_num;
  uint64_t contain_group_num;
  uint64_t match_group_num;
  uint64_t collide_group_num;
  uint64_t check_rule_num;

  void Init() {
    check_tuple_num = 0;
    contain_group_num = 0;
    match_group_num = 0;
    collide_group_num = 0;
    check_rule_num = 0;
  }
};

struct DtInfo {
  vector<DtRule *> rules;
  int rules_num;

  // Init
  uint64_t ip_mask[33];
  uint64_t ip_mask_pair[33][33];
  uint32_t ip_num[33][33];
  uint32_t ip_num_sum[33][33];
  uint64_t max_priority[33][33];
  bool pre_tuples_prefix[33][33];

  // X Y
  uint64_t check_tuple_num[33][33];
  uint64_t group_vis[33][33];
  uint64_t contain_group_num[33][33];
  uint64_t match_group_num[33][33];
  uint64_t collide_group_num[33][33];
  uint64_t check_rule_num[33][33];

  struct TupleCost tuple_cost[33][33][33][33];
#ifdef TUPLEINFO
  struct TupleInfo tuple_info[33][33][33][33];
#endif

  void GetDtRules(vector<Rule_DT_MT *> &_rules);
  void Init(vector<TupleRange> pre_tuple_ranges);
  uint32_t GetIpNumSum(int x1, int y1, int x2, int y2);
  void DtCalculate(bool accelerate);
  void DtCalculateXY(int x1, int y1, bool accelerate);
  void ReducePreTupleRanges(vector<TupleRange> pre_tuple_ranges);
  void DynamicProgramming();
  void DynamicProgrammingXY(int x1, int y1);
  vector<TupleRange> GetTupleRanges(int x1, int y1, int x2, int y2);
  void AddTupleRanges(vector<TupleRange> &tuple_ranges,
                      TupleInfo &tuple_info_sum, int x1, int y1, int x2,
                      int y2);
  void TupleInfoSum(TupleInfo &tuple_info_sum, int x1, int y1, int x2, int y2);
  int Free();
};

vector<TupleRange> TupleRangeStep(int step);
vector<TupleRange> DynamicTupleRanges(vector<Rule_DT_MT *> &rules,
                                      double &drs_time,
                                      vector<TupleRange> pre_tuple_ranges);
void PrintTupleRanges(vector<TupleRange> &tuple_ranges);

#endif
