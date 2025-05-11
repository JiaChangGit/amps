#ifndef DYNAMICTUPLEDIMS_H
#define DYNAMICTUPLEDIMS_H

#include <cmath>

#include "elementary_DT_MT.h"
#include "io_DT_MT.h"

struct DimRange {
  int x1, x2;
};

struct DimsRange {
  vector<DimRange> dims[5];
  char reduced[5][33];

  void Init();
};

struct DimKey {
  uint32_t key[5];
  bool operator<(const DimKey &rule) const {
    for (int i = 0; i < 4; ++i)
      if (key[i] != rule.key[i]) return key[i] < rule.key[i];
    return key[4] < rule.key[4];
  }
};

struct DimRule {
  uint32_t key[5];
  uint32_t reduced_key[5];
  char prefix_len[5];
  int priority;
  int check_num;
  bool first;
};

struct DimInfo {
  vector<DimRule *> rules;
  int rules_num;
  DimsRange dims_range;

  int dim_len_rules_num[33];
  int dim_check_tuple_num[33];
  map<uint32_t, uint32_t> tuples_priority;
  uint64_t tuples_priority_sum;
  uint64_t dim_check_tuples_num[33][33];
  uint64_t dim_check_rules_num[33][33];
  uint64_t tuple_cost_sum[33][33];
  uint64_t dp_cost[33][33];
  uint32_t dp_tuple[33][33];
  uint32_t dp_rule[33][33];
  map<uint32_t, map<DimKey, uint32_t>> tuples_key[33];

  void GetDimRules(vector<Rule_DT_MT *> &rules);
  uint64_t CalculateDimRange(int dim);
  void CalculateDimRangeX(int dim, int x);
  void DynamicProgrammingX(int dim);
  void GetDpRange(int i, int j, int dim);
  void Free();
};

DimsRange GetDimRange(vector<Rule_DT_MT *> &rules, int prefix_dims_num);
#endif
