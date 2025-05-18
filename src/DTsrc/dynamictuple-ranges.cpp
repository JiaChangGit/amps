#include "dynamictuple-ranges.h"

#include <cmath>
using namespace std;

uint64_t check_hash_cost = 5;
uint64_t check_group_cost = 2;
uint64_t check_rule_cost = 3;

bool dt_print_info = false;
bool dt_print_num = false;

TupleInfo tuple_info_sum;

vector<TupleRange> TupleRangeStep(int step) {
  vector<TupleRange> tuple_ranges;
  for (int x1 = 0; x1 <= 32; x1 += step)
    for (int y1 = 0; y1 <= 32; y1 += step) {
      int x2 = min(x1 + step - 1, 32);
      int y2 = min(y1 + step - 1, 32);
      TupleRange tuple_range(x1, y1, x2, y2);
      tuple_ranges.push_back(tuple_range);
    }
  return tuple_ranges;
}

void DtInfo::GetDtRules(vector<Rule_DT_MT *> &_rules) {
  rules.clear();
  rules_num = _rules.size();
  sort(_rules.begin(), _rules.end(), CmpRulePriority);
  for (int i = 0; i < rules_num; ++i) {
    DtRule *rule = (DtRule *)malloc(sizeof(DtRule));
    rule->src_dst_ip =
        (uint64_t)_rules[i]->range[0][0] << 32 | _rules[i]->range[1][0];
    rule->src_prefix_len = _rules[i]->prefix_len[0];
    rule->dst_prefix_len = _rules[i]->prefix_len[1];
    rule->priority = rules_num - i;
    rules.push_back(rule);
  }
}

void DtInfo::Init(vector<TupleRange> pre_tuple_ranges) {
  // calculate mask
  ip_mask[0] = 0;
  for (int i = 1; i <= 32; ++i)
    ip_mask[i] = ip_mask[i - 1] | (1ULL << (32 - i));
  for (int i = 0; i <= 32; ++i)
    for (int j = 0; j <= 32; ++j)
      ip_mask_pair[i][j] = ip_mask[i] << 32 | ip_mask[j];

  // calculate ip_num and priority
  memset(ip_num, 0, sizeof(ip_num));
  memset(max_priority, 0, sizeof(max_priority));
  for (int i = rules_num - 1; i >= 0; --i) {
    int x = rules[i]->src_prefix_len;
    int y = rules[i]->dst_prefix_len;
    ++ip_num[x][y];
    max_priority[x][y] = rules[i]->priority;
  }

  for (int x = 0; x <= 32; ++x)
    for (int y = 0; y <= 32; ++y) {
      ip_num_sum[x][y] = ip_num[x][y];
      if (x > 0) ip_num_sum[x][y] += ip_num_sum[x - 1][y];
      if (y > 0) ip_num_sum[x][y] += ip_num_sum[x][y - 1];
      if (x > 0 && y > 0) ip_num_sum[x][y] -= ip_num_sum[x - 1][y - 1];
    }
  memset(pre_tuples_prefix, 0, sizeof(pre_tuples_prefix));
  for (int i = 0; i < pre_tuple_ranges.size(); ++i)
    pre_tuples_prefix[pre_tuple_ranges[i].x1][pre_tuple_ranges[i].y1] = 1;

  if (dt_print_num) {
    for (int y = 32; y >= 0; --y) {
      for (int x = 0; x <= 32; ++x) {
        std::cout << ip_num[x][y] << ' ';
      }
      std::cout << '\n';
    }
    std::cout << '\n';
  }
}

uint32_t DtInfo::GetIpNumSum(int x1, int y1, int x2, int y2) {
  uint64_t ans = ip_num_sum[x2][y2];
  if (x1 > 0) ans -= ip_num_sum[x1 - 1][y2];
  if (y1 > 0) ans -= ip_num_sum[x2][y1 - 1];
  if (x1 > 0 && y1 > 0) ans += ip_num_sum[x1 - 1][y1 - 1];
  return ans;
}

bool CmpDrsRulesIpPriority(DtRule *drs_rule1, DtRule *drs_rule2) {
  if (drs_rule1->reduced_src_dst_ip != drs_rule2->reduced_src_dst_ip)
    return drs_rule1->reduced_src_dst_ip < drs_rule2->reduced_src_dst_ip;
  return drs_rule1->priority > drs_rule2->priority;
}

void DtInfo::DtCalculateXY(int x1, int y1, bool accelerate) {
  if (accelerate &&
      (GetIpNumSum(x1, y1, x1, 32) == 0 || GetIpNumSum(x1, y1, 32, y1) == 0)) {
    for (int x2 = x1; x2 <= 32; ++x2)
      for (int y2 = y1; y2 <= 32; ++y2) {
        if (GetIpNumSum(x1, y1, x2, y2) == 0)
          tuple_cost[x1][y1][x2][y2].cost = 0;
        else
          tuple_cost[x1][y1][x2][y2].cost = 1e9;
        tuple_cost[x1][y1][x2][y2].split_type = SPLIT_SELF;
      }
    return;
  }

  // reduce rule ip
  for (int i = 0; i < rules_num; ++i)
    rules[i]->reduced_src_dst_ip = rules[i]->src_dst_ip & ip_mask_pair[x1][y1];
  sort(rules.begin(), rules.end(), CmpDrsRulesIpPriority);

  // tuple
  check_tuple_num[x1][y1] = max_priority[x1][y1];
  for (int x = x1 + 1; x <= 32; ++x)
    check_tuple_num[x][y1] =
        max(check_tuple_num[x - 1][y1], max_priority[x][y1]);
  for (int y = y1 + 1; y <= 32; ++y)
    check_tuple_num[x1][y] =
        max(check_tuple_num[x1][y - 1], max_priority[x1][y]);
  for (int x = x1 + 1; x <= 32; ++x)
    for (int y = y1 + 1; y <= 32; ++y)
      check_tuple_num[x][y] =
          max(max(check_tuple_num[x - 1][y], check_tuple_num[x][y - 1]),
              max_priority[x][y]);

  // group && rule
  rules[rules_num - 1]->check_num = 1;
  for (int i = rules_num - 2; i >= 0; --i)
    if (rules[i]->reduced_src_dst_ip == rules[i + 1]->reduced_src_dst_ip) {
      rules[i + 1]->first = false;
      rules[i]->check_num = rules[i + 1]->check_num + 1;
    } else {
      rules[i + 1]->first = true;
      rules[i]->check_num = 1;
    }
  rules[0]->first = true;

  int group_num = 0;
  memset(group_vis, 0, sizeof(group_vis));
  memset(contain_group_num, 0, sizeof(contain_group_num));
  memset(match_group_num, 0, sizeof(match_group_num));
  memset(collide_group_num, 0, sizeof(collide_group_num));
  memset(check_rule_num, 0, sizeof(check_rule_num));

  // match_group_num
  for (int i = 0; i < rules_num; ++i) {
    int px = rules[i]->src_prefix_len;
    int py = rules[i]->dst_prefix_len;
    if (rules[i]->first) ++group_num;
    if (px < x1 || py < y1) continue;
    for (int x = px; x <= 32; ++x) {
      if (group_vis[x][y1] == group_num) break;
      for (int y = py; y <= 32; ++y) {
        if (group_vis[x][y] == group_num) break;
        group_vis[x][y] = group_num;
        contain_group_num[x][y] += 1;
        match_group_num[x][y] += rules[i]->check_num;
        collide_group_num[x][y] += rules[i]->priority - rules[i]->check_num;
      }
    }
    check_rule_num[px][py] += rules[i]->check_num;
  }

  // collide_group_num
  for (int x2 = x1; x2 <= 32; ++x2)
    for (int y2 = y1; y2 <= 32; ++y2) {
      int bucket_size = 32;
      while (bucket_size * 0.85 <= contain_group_num[x2][y2]) bucket_size <<= 1;
      collide_group_num[x2][y2] = 1.0 * collide_group_num[x2][y2] / bucket_size;
    }

  // check_rule_num
  for (int x = x1 + 1; x <= 32; ++x)
    check_rule_num[x][y1] += check_rule_num[x - 1][y1];
  for (int y = y1 + 1; y <= 32; ++y)
    check_rule_num[x1][y] += check_rule_num[x1][y - 1];
  for (int x = x1 + 1; x <= 32; ++x)
    for (int y = y1 + 1; y <= 32; ++y)
      check_rule_num[x][y] += check_rule_num[x][y - 1] +
                              check_rule_num[x - 1][y] -
                              check_rule_num[x - 1][y - 1];

  // summary

  for (int x2 = x1; x2 <= 32; ++x2)
    for (int y2 = y1; y2 <= 32; ++y2) {
      tuple_cost[x1][y1][x2][y2].cost =
          check_tuple_num[x2][y2] * check_hash_cost +
          (match_group_num[x2][y2] + collide_group_num[x2][y2]) *
              check_group_cost +
          check_rule_num[x2][y2] * check_rule_cost;
      tuple_cost[x1][y1][x2][y2].split_type = SPLIT_SELF;
    }

#ifdef TUPLEINFO
  for (int x2 = x1; x2 <= 32; ++x2)
    for (int y2 = y1; y2 <= 32; ++y2) {
      tuple_info[x1][y1][x2][y2].check_tuple_num = check_tuple_num[x2][y2];
      tuple_info[x1][y1][x2][y2].contain_group_num = contain_group_num[x2][y2];
      tuple_info[x1][y1][x2][y2].match_group_num = match_group_num[x2][y2];
      tuple_info[x1][y1][x2][y2].collide_group_num = collide_group_num[x2][y2];
      tuple_info[x1][y1][x2][y2].check_rule_num = check_rule_num[x2][y2];
    }
#endif
}

void DtInfo::DtCalculate(bool accelerate) {
  for (int x1 = 32; x1 >= 0; --x1)
    for (int y1 = 32; y1 >= 0; --y1)
      DtCalculateXY(x1, y1, accelerate && !pre_tuples_prefix[x1][y1]);
}

void DtInfo::ReducePreTupleRanges(vector<TupleRange> pre_tuple_ranges) {
  for (int i = 0; i < pre_tuple_ranges.size(); ++i) {
    TupleRange tuple_range = pre_tuple_ranges[i];
    tuple_cost[tuple_range.x1][tuple_range.y1][tuple_range.x2][tuple_range.y2]
        .cost *= 0.9;
  }
}

void DtInfo::DynamicProgrammingXY(int x1, int y1) {
  for (int x2 = x1; x2 <= 32; ++x2) {
    for (int y2 = y1; y2 <= 32; ++y2) {
      if (tuple_cost[x1][y1][x2][y2].cost == 0) continue;
      if (tuple_cost[x2][y1][x2][y2].cost == 0) {
        tuple_cost[x1][y1][x2][y2] = tuple_cost[x1][y1][x2 - 1][y2];
        continue;
      }
      if (tuple_cost[x1][y2][x2][y2].cost == 0) {
        tuple_cost[x1][y1][x2][y2] = tuple_cost[x1][y1][x2][y2 - 1];
        continue;
      }
      if (tuple_cost[x1][y1][x1][y2].cost == 0) {
        if (tuple_cost[x1 + 1][y1][x1 + 1][y2].cost != 0) {
          tuple_cost[x1][y1][x2][y2].cost = tuple_cost[x1 + 1][y1][x2][y2].cost;
          tuple_cost[x1][y1][x2][y2].split_type = SPLIT_SRC;
          tuple_cost[x1][y1][x2][y2].split_prefix_len = x1;
        } else {
          tuple_cost[x1][y1][x2][y2] = tuple_cost[x1 + 1][y1][x2][y2];
        }
        continue;
      }
      if (tuple_cost[x1][y1][x2][y1].cost == 0) {
        if (tuple_cost[x1][y1 + 1][x2][y1 + 1].cost != 0) {
          tuple_cost[x1][y1][x2][y2].cost = tuple_cost[x1][y1 + 1][x2][y2].cost;
          tuple_cost[x1][y1][x2][y2].split_type = SPLIT_DST;
          tuple_cost[x1][y1][x2][y2].split_prefix_len = y1;
        } else {
          tuple_cost[x1][y1][x2][y2] = tuple_cost[x1][y1 + 1][x2][y2];
        }
        continue;
      }

      for (int x = x1; x < x2; ++x) {
        uint32_t new_cost =
            tuple_cost[x1][y1][x][y2].cost + tuple_cost[x + 1][y1][x2][y2].cost;
        if (new_cost < tuple_cost[x1][y1][x2][y2].cost) {
          tuple_cost[x1][y1][x2][y2].cost = new_cost;
          tuple_cost[x1][y1][x2][y2].split_type = SPLIT_SRC;
          tuple_cost[x1][y1][x2][y2].split_prefix_len = x;
        }
      }

      for (int y = y1; y < y2; ++y) {
        uint32_t new_cost =
            tuple_cost[x1][y1][x2][y].cost + tuple_cost[x1][y + 1][x2][y2].cost;
        if (new_cost < tuple_cost[x1][y1][x2][y2].cost) {
          tuple_cost[x1][y1][x2][y2].cost = new_cost;
          tuple_cost[x1][y1][x2][y2].split_type = SPLIT_DST;
          tuple_cost[x1][y1][x2][y2].split_prefix_len = y;
        }
      }
    }
  }
}

void DtInfo::DynamicProgramming() {
  for (int x1 = 32; x1 >= 0; --x1)
    for (int y1 = 32; y1 >= 0; --y1) DynamicProgrammingXY(x1, y1);
}

void DtInfo::TupleInfoSum(TupleInfo &tuple_info_sum, int x1, int y1, int x2,
                          int y2) {
#ifdef TUPLEINFO
  int check_tuple_num = tuple_info[x1][y1][x2][y2].check_tuple_num;
  int contain_group_num = tuple_info[x1][y1][x2][y2].contain_group_num;
  int match_group_num = tuple_info[x1][y1][x2][y2].match_group_num;
  int collide_group_num = tuple_info[x1][y1][x2][y2].collide_group_num;
  int check_rule_num = tuple_info[x1][y1][x2][y2].check_rule_num;
  int ip_num = GetIpNumSum(x1, y1, x2, y2);
  int tuple_cost_num = tuple_cost[x1][y1][x2][y2].cost;

  tuple_info_sum.check_tuple_num += check_tuple_num;
  tuple_info_sum.contain_group_num += contain_group_num;
  tuple_info_sum.match_group_num += match_group_num;
  tuple_info_sum.collide_group_num += collide_group_num;
  tuple_info_sum.check_rule_num += check_rule_num;
  if (dt_print_info) {
    std::cout << x1 << ", " << y1 << ", " << x2 << ", " << y2 << " \\\\  ";

    std::cout << "check_tuple_num " << check_tuple_num << " contain_group_num "
              << contain_group_num << " match_group_num " << match_group_num
              << " collide_group_num " << collide_group_num
              << " check_rule_num " << check_rule_num << "  ";

    std::cout << "ip_num " << ip_num << " tuple_cost_num " << tuple_cost_num
              << '\n';
  }
#endif
}
void DtInfo::AddTupleRanges(vector<TupleRange> &tuple_ranges,
                            TupleInfo &tuple_info_sum, int x1, int y1, int x2,
                            int y2) {
  if (tuple_cost[x1][y1][x2][y2].split_type == SPLIT_SELF) {
    TupleRange tuple_range(x1, y1, x2, y2);
    tuple_ranges.push_back(tuple_range);
    TupleInfoSum(tuple_info_sum, x1, y1, x2, y2);
  } else if (tuple_cost[x1][y1][x2][y2].split_type == SPLIT_SRC) {
    int x = tuple_cost[x1][y1][x2][y2].split_prefix_len;
    AddTupleRanges(tuple_ranges, tuple_info_sum, x1, y1, x, y2);
    AddTupleRanges(tuple_ranges, tuple_info_sum, x + 1, y1, x2, y2);
  } else if (tuple_cost[x1][y1][x2][y2].split_type == SPLIT_DST) {
    int y = tuple_cost[x1][y1][x2][y2].split_prefix_len;
    AddTupleRanges(tuple_ranges, tuple_info_sum, x1, y1, x2, y);
    AddTupleRanges(tuple_ranges, tuple_info_sum, x1, y + 1, x2, y2);
  }
}

vector<TupleRange> DtInfo::GetTupleRanges(int x1, int y1, int x2, int y2) {
  vector<TupleRange> tuple_ranges;
  tuple_info_sum.Init();
  AddTupleRanges(tuple_ranges, tuple_info_sum, x1, y1, x2, y2);
#ifdef TUPLEINFO
  if (dt_print_info) {
    std::cout << "check_tuple_sum " << tuple_info_sum.check_tuple_num << '\n'
              << "contain_group_sum " << tuple_info_sum.contain_group_num
              << '\n'
              << "match_group_sum " << tuple_info_sum.match_group_num << '\n'
              << "collide_group_sum " << tuple_info_sum.collide_group_num
              << '\n'
              << "check_rule_sum " << tuple_info_sum.check_rule_num << '\n'
              << "tuple_cost_sum " << tuple_cost[0][0][32][32].cost << '\n';
  }
#endif
  return tuple_ranges;
}

int DtInfo::Free() {
  for (int i = 0; i < rules_num; ++i) free(rules[i]);
  rules.clear();
  free(this);
  return 0;
}

vector<TupleRange> DynamicTupleRanges(vector<Rule_DT_MT *> &rules,
                                      double &dt_time,
                                      vector<TupleRange> pre_tuple_ranges) {
  if (rules.size() == 0) return TupleRangeStep(8);

  timeval timeval_start, timeval_end;
  gettimeofday(&timeval_start, NULL);

  DtInfo *dt_info = new DtInfo();
  dt_info->GetDtRules(rules);
  dt_info->Init(pre_tuple_ranges);
  dt_info->DtCalculate(true);
  dt_info->ReducePreTupleRanges(pre_tuple_ranges);
  dt_info->DynamicProgramming();
  vector<TupleRange> tuple_ranges = dt_info->GetTupleRanges(0, 0, 32, 32);
  dt_info->Free();

  gettimeofday(&timeval_end, NULL);
  dt_time = GetRunTimeUs(timeval_start, timeval_end) / 1000000.0;

  return tuple_ranges;
}

void PrintTupleRanges(vector<TupleRange> &tuple_ranges) {
  int tuple_ranges_num = tuple_ranges.size();
  std::cout << tuple_ranges_num << '\n';

  for (int i = 0; i < tuple_ranges_num; ++i) {
    const auto &r = tuple_ranges[i];
    std::cout << r.x1 << ' ' << r.y1 << ' ' << r.x2 << ' ' << r.y2 << '\n';
  }
}
