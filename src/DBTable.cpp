#include "DBT_core.hpp"

namespace DBT {
uint64_t check_hash_cost = 5;
uint64_t check_group_cost = 2;
uint64_t check_rule_cost = 3;
bool dt_print_info = false;
bool dt_print_num = false;

/// ====== ///
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
vector<TupleRange> DynamicTupleRanges(vector<Rule*>& rules, double& dt_time,
                                      vector<TupleRange> pre_tuple_ranges) {
  if (rules.size() == 0) return TupleRangeStep(8);

  DtInfo* dt_info = new DtInfo();
  dt_info->GetDtRules(rules);
  // 初始化区间元组
  dt_info->Init(pre_tuple_ranges);
  dt_info->DtCalculate(true);
  dt_info->ReducePreTupleRanges(pre_tuple_ranges);
  dt_info->DynamicProgramming();
  vector<TupleRange> tuple_ranges = dt_info->GetTupleRanges(0, 0, 32, 32);
  dt_info->Free();

  return tuple_ranges;
}

void PrintTupleRanges(const vector<TupleRange>& tuple_ranges) {
  const size_t tuple_ranges_num = tuple_ranges.size();
  std::cout << tuple_ranges_num << "\n";
  for (int i = 0; i < tuple_ranges_num; ++i)
    std::cout << tuple_ranges[i].x1 << " " << tuple_ranges[i].y1 << " "
              << tuple_ranges[i].x2 << " " << tuple_ranges[i].y2 << "\n";
}
/// ====== ///

//// ==== ////
void DtInfo::GetDtRules(vector<Rule*>& _rules) {
  rules.clear();
  rules_num = _rules.size();
  sort(_rules.begin(), _rules.end(),
       [](Rule* a, Rule* b) -> bool { return a->pri < b->pri; });
  for (int i = 0; i < rules_num; ++i) {
    DtRule* rule = (DtRule*)malloc(sizeof(DtRule));
    rule->src_dst_ip =
        (uint64_t)(_rules[i]->ip.i_32.sip & maskBit[_rules[i]->sip_length])
            << 32 |
        (_rules[i]->ip.i_32.dip & maskBit[_rules[i]->dip_length]);
    rule->src_prefix_len = _rules[i]->sip_length;
    rule->dst_prefix_len = _rules[i]->dip_length;
    rule->priority = rules_num - i;
    rules.push_back(rule);
  }
}

void DtInfo::Init(const vector<TupleRange>& pre_tuple_ranges) {
  // calculate mask
  ip_mask[0] = 0;
  for (int i = 1; i <= 32; ++i)
    ip_mask[i] = ip_mask[i - 1] | (1ULL << (32 - i));
  for (int i = 0; i <= 32; ++i)
    for (int j = 0; j <= 32; ++j)
      ip_mask_pair[i][j] = ip_mask[i] << 32 | ip_mask[j];

  // calculate ip_num and priority
  //
  memset(ip_num, 0, sizeof(ip_num));
  //
  memset(max_priority, 0, sizeof(max_priority));
  //
  for (int i = rules_num - 1; i >= 0; --i) {
    int x = rules[i]->src_prefix_len;
    int y = rules[i]->dst_prefix_len;
    ++ip_num[x][y];
    max_priority[x][y] = rules[i]->priority;
  }
  //
  for (int x = 0; x <= 32; ++x)
    for (int y = 0; y <= 32; ++y) {
      ip_num_sum[x][y] = ip_num[x][y];
      if (x > 0) ip_num_sum[x][y] += ip_num_sum[x - 1][y];
      if (y > 0) ip_num_sum[x][y] += ip_num_sum[x][y - 1];
      if (x > 0 && y > 0) ip_num_sum[x][y] -= ip_num_sum[x - 1][y - 1];
    }
  //
  memset(pre_tuples_prefix, 0, sizeof(pre_tuples_prefix));
  //
  const size_t pre_tuple_rangesNum = pre_tuple_ranges.size();
  for (int i = 0; i < pre_tuple_rangesNum; ++i)
    pre_tuples_prefix[pre_tuple_ranges[i].x1][pre_tuple_ranges[i].y1] = 1;

  if (dt_print_num) {
    for (int y = 32; y >= 0; --y) {
      for (int x = 0; x <= 32; ++x) std::cout << ip_num[x][y] << " ";

      std::cout << ("\n");
    }
    std::cout << ("\n");
  }
}

//
uint32_t DtInfo::GetIpNumSum(int x1, int y1, int x2, int y2) {
  uint64_t ans = ip_num_sum[x2][y2];
  if (x1 > 0) ans -= ip_num_sum[x1 - 1][y2];
  if (y1 > 0) ans -= ip_num_sum[x2][y1 - 1];
  if (x1 > 0 && y1 > 0) ans += ip_num_sum[x1 - 1][y1 - 1];
  return ans;
}

bool CmpDrsRulesIpPriority(DtRule* drs_rule1, DtRule* drs_rule2) {
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

void DtInfo::ReducePreTupleRanges(const vector<TupleRange>& pre_tuple_ranges) {
  const size_t pre_tuple_rangesNum = pre_tuple_ranges.size();
  for (int i = 0; i < pre_tuple_rangesNum; ++i) {
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

void DtInfo::TupleInfoSum(TupleInfo& tuple_info_sum, int x1, int y1, int x2,
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
              << "\n";
  }
#endif
}
void DtInfo::AddTupleRanges(vector<TupleRange>& tuple_ranges,
                            TupleInfo& tuple_info_sum, int x1, int y1, int x2,
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
    std::cout << "check_tuple_sum " << tuple_info_sum.check_tuple_num << "\n";
    std::cout << "contain_group_sum " << tuple_info_sum.contain_group_num
              << "\n";
    std::cout << "match_group_sum " << tuple_info_sum.match_group_num << "\n";
    std::cout << "collide_group_sum " << tuple_info_sum.collide_group_num
              << "\n";
    std::cout << "check_rule_sum " << tuple_info_sum.check_rule_num << "\n";
    std::cout << "tuple_cost_sum " << tuple_cost[0][0][32][32].cost << "\n";
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
//// ==== ////

/// === ///

uint32_t hashCode(uint32_t hash1, uint32_t hash2) {
  hash1 ^= hash1 >> 16;
  hash1 *= 0x85ebca6b;
  hash1 ^= hash1 >> 13;
  hash1 *= 0xc2b2ae35;

  hash2 ^= hash2 >> 16;
  hash2 *= 0x85ebca6b;
  hash2 ^= hash2 >> 13;
  hash2 *= 0xc2b2ae35;

  hash1 ^= hash2;
  hash1 ^= hash1 >> 16;

  return hash1;
}

uint32_t hashCode(uint64_t hash) {
  uint32_t hash1 = (hash & 0xFFFFFFFF00000000) >> 32;
  uint32_t hash2 = hash & 0x00000000FFFFFFFF;

  hash1 ^= hash1 >> 16;
  hash1 *= 0x85ebca6b;
  hash1 ^= hash1 >> 13;
  hash1 *= 0xc2b2ae35;

  hash2 ^= hash2 >> 16;
  hash2 *= 0x85ebca6b;
  hash2 ^= hash2 >> 13;
  hash2 *= 0xc2b2ae35;

  hash1 ^= hash2;
  hash1 ^= hash1 >> 16;

  return hash1;
}
/// === ///

///// ===== /////
CacuInfo::CacuInfo(vector<Rule*>& _rules, int _threshold)
    : rules(_rules),
      threshold(_threshold),
      bucket_num(0),
      max_bucket_size(0),
      target_bucket_num(0) {}

void CacuInfo::CacuIpMask(MASK& mask) {
  mask.i_64 = 0;
  vector<CacuRule*> _rules;
  for (auto& _r : rules) {
    // CacuRule* rule = (CacuRule*)malloc(sizeof(CacuRule));
    CacuRule* rule = new CacuRule;
    memcpy(rule, _r, sizeof(Rule));
    _rules.emplace_back(rule);
  }
  _rules[0]->is_first = true;
  const size_t rulesNum = _rules.size();
  _rules[0]->size = rulesNum;

  int dbs_size = 0;

  while (true) {
    BitRank bRank[64] = {0};
    for (int i = 0; i < 64; ++i) bRank[i].id = i;
    for (int i = 0; i < rulesNum;) {
      fetch_bit_by_ip(i, _rules[i]->size, bRank, _rules);
      i += _rules[i]->size;
    }
    sort(bRank, bRank + 64, [](BitRank a, BitRank b) -> bool {
      if (a.count != b.count)
        return a.count > b.count;
      else
        return a.rank < b.rank;
    });
    // cout << bRank[0].id << "\n";
    int old_target_bucket_num = target_bucket_num;
    int old_max_bucket_size = max_bucket_size;
    int old_bucket_num = bucket_num;
    partition_by_ip(bRank[0].id, _rules);
    if (target_bucket_num == old_target_bucket_num &&
        old_max_bucket_size == max_bucket_size &&
        old_bucket_num == bucket_num) {
      // std::cout << "480\n";  //// JIA
      break;
    }
    if (bRank[0].id < 32)
      mask.i_32.smask |= getBit[bRank[0].id];
    else
      mask.i_32.dmask |= getBit[bRank[0].id - 32];
    // fetched_bit_id.emplace_back(bRank[0].id);
    ++dbs_size;
    if (((double)target_bucket_num / (double)bucket_num) > END_BOUND) {
      // std::cout << "490\n";  //// JIA
      break;
    }
  }

  // print_bucket(_rules);
  cout << "\nDBS bitsNum: " << dbs_size << "\n\n";

  for (auto _r : _rules) delete _r;
}

void CacuInfo::fetch_bit_by_ip(int start, int size, BitRank bRank[],
                               vector<CacuRule*>& _rules) {
  int sipBitCount[32][3] = {0};
  int dipBitCount[32][3] = {0};
  for (int j = start; j < start + size; ++j) {
    for (int i = 0; i < 32; ++i) {
      if (i > _rules[j]->sip_length - 1) {
        ++sipBitCount[i][2];
      } else {
        if (getBit[i] & _rules[j]->ip.i_32.sip)
          ++sipBitCount[i][1];
        else
          ++sipBitCount[i][0];
      }
    }
    for (int i = 0; i < 32; ++i) {
      if (i > _rules[j]->dip_length - 1) {
        ++dipBitCount[i][2];
      } else {
        if (getBit[i] & _rules[j]->ip.i_32.dip)
          ++dipBitCount[i][1];
        else
          ++dipBitCount[i][0];
      }
    }
  }
  BitRank tmpRank[64] = {0};
  for (int i = 0; i < 32; ++i) {
    tmpRank[i].id = i;
    tmpRank[i].rank = (double)(abs(sipBitCount[i][0] - sipBitCount[i][1]) +
                               sipBitCount[i][2]) /
                      size;
  }
  for (int i = 0; i < 32; ++i) {
    tmpRank[i + 32].id = i + 32;
    tmpRank[i + 32].rank = (double)(abs(dipBitCount[i][0] - dipBitCount[i][1]) +
                                    dipBitCount[i][2]) /
                           size;
  }

  sort(tmpRank, tmpRank + 64,
       [](BitRank a, BitRank b) -> bool { return a.rank < b.rank; });

  for (int i = 0; i < TOP_K; ++i) {
    bRank[tmpRank[i].id].count += size;
    bRank[tmpRank[i].id].rank += tmpRank[i].rank;
  }
}

void CacuInfo::partition_by_ip(int bit_id, vector<CacuRule*>& _rules) {
  MASK _mask = {0};
  if (bit_id < 32)
    _mask.i_32.smask = getBit[bit_id];
  else
    _mask.i_32.dmask = getBit[bit_id - 32];

  for (auto& _r : _rules) {
    if ((bit_id < 32 && _r->sip_length - 1 < bit_id) ||
        (bit_id >= 32 && _r->dip_length - 1 < (bit_id - 32))) {
      _r->fetch_bit = 2;
    } else if (_r->ip.i_64 & _mask.i_64) {
      _r->fetch_bit = 1;
    } else {
      _r->fetch_bit = 0;
    }
  }
  const size_t rulesNum = _rules.size();

  for (int i = 0; i < rulesNum;) {
    int _step = _rules[i]->size;
    partition_in_bucket(i, _step, _rules);
    i += _step;
  }

  // print_bucket(_rules);

  int target_num = 0;
  int max_size = 0;
  int total_num = 0;
  for (auto _r : _rules) {
    if (_r->is_first) {
      ++total_num;
      if (_r->size < threshold) ++target_num;
      if (max_size < _r->size) max_size = _r->size;
    }
  }
  if (target_num != target_bucket_num || max_size != max_bucket_size ||
      total_num != bucket_num) {
    target_bucket_num = target_num;
    bucket_num = total_num;
    max_bucket_size = max_size;
  }
}

void CacuInfo::partition_in_bucket(int start, int size,
                                   vector<CacuRule*>& _rules) {
  sort(_rules.begin() + start, _rules.begin() + start + size,
       [](CacuRule* a, CacuRule* b) -> bool {
         if (a->fetch_bit == b->fetch_bit)
           return a->pri < b->pri;
         else
           return a->fetch_bit < b->fetch_bit;
       });

  _rules[start + size - 1]->size = 1;
  for (int i = start + size - 2; i >= start; --i) {
    if (_rules[i]->fetch_bit == _rules[i + 1]->fetch_bit) {
      _rules[i + 1]->is_first = false;
      _rules[i]->size = _rules[i + 1]->size + 1;
    } else {
      _rules[i + 1]->is_first = true;
      _rules[i]->size = 1;
    }
  }
  _rules[start]->is_first = true;
}

uint16_t CacuInfo::CacuPortMask(int type) {
  uint16_t mask = 0;
  vector<CacuRule*> _rules;
  for (auto& _r : rules) {
    CacuRule* rule = new CacuRule;
    memcpy(rule, _r, sizeof(Rule));
    _rules.emplace_back(rule);
  }
  _rules[0]->is_first = true;
  const size_t rulesNum = _rules.size();
  _rules[0]->size = rulesNum;

  while (true) {
    BitRank bRank[16] = {0};
    for (int i = 0; i < 16; ++i) bRank[i].id = i;
    for (int i = 0; i < rulesNum;) {
      fetch_bit_by_port(type, i, _rules[i]->size, bRank, _rules);
      i += _rules[i]->size;
    }
    sort(bRank, bRank + 16, [](BitRank a, BitRank b) -> bool {
      if (a.count != b.count)
        return a.count > b.count;
      else
        return a.rank < b.rank;
    });
    int old_target_bucket_num = target_bucket_num;
    int old_max_bucket_size = max_bucket_size;
    int old_bucket_num = bucket_num;
    partition_by_port(type, bRank[0].id, _rules);
    if (target_bucket_num == old_target_bucket_num &&
        old_max_bucket_size == max_bucket_size && old_bucket_num == bucket_num)
      break;
    mask |= (uint16_t)getBit[bRank[0].id + 16];
    // fetched_bit_id.emplace_back(bRank[0].id);
    if (((double)target_bucket_num / (double)bucket_num) > END_BOUND) break;
  }

  // print_bucket(_rules);
  // sort(fetched_bit_id.begin(), fetched_bit_id.end());
  // for (auto x : fetched_bit_id)cout << x << " ";
  // cout << "\n" << "\n";

  for (auto& _r : _rules) delete _r;

  return mask;
}

void CacuInfo::fetch_bit_by_port(int type, int start, int size, BitRank bRank[],
                                 vector<CacuRule*>& _rules) {
  int portBitCount[16][2] = {0};
  for (int j = start; j < start + size; ++j) {
    for (int i = 0; i < 16; ++i) {
      if (_rules[j]->Port[type][0] & getBit[i + 16])
        ++portBitCount[i][1];
      else
        ++portBitCount[i][0];
    }
  }
  BitRank tmpRank[16] = {0};
  for (int i = 0; i < 16; ++i) {
    tmpRank[i].id = i;
    tmpRank[i].rank =
        (double)(abs(portBitCount[i][0] - portBitCount[i][1])) / size;
  }

  sort(tmpRank, tmpRank + 16,
       [](BitRank a, BitRank b) -> bool { return a.rank < b.rank; });

  for (int i = 0; i < TOP_K; ++i) {
    bRank[tmpRank[i].id].count += size;
    bRank[tmpRank[i].id].rank += tmpRank[i].rank;
  }
}

void CacuInfo::partition_by_port(int type, int bit_id,
                                 vector<CacuRule*>& _rules) {
  uint16_t _mask = getBit[bit_id + 16];

  for (auto& _r : _rules) {
    if (_r->Port[type][0] & _mask)
      _r->fetch_bit = 1;
    else
      _r->fetch_bit = 0;
  }
  const size_t rulesNum = _rules.size();
  for (int i = 0; i < rulesNum;) {
    int _step = _rules[i]->size;
    partition_in_bucket(i, _step, _rules);
    i += _step;
  }

  // print_bucket(_rules);

  int target_num = 0;
  int max_size = 0;
  int total_num = 0;
  for (auto _r : _rules) {
    if (_r->is_first) {
      ++total_num;
      if (_r->size < threshold) ++target_num;
      if (max_size < _r->size) max_size = _r->size;
    }
  }
  if (target_num != target_bucket_num || max_size != max_bucket_size ||
      total_num != bucket_num) {
    target_bucket_num = target_num;
    bucket_num = total_num;
    max_bucket_size = max_size;
  }
}

void CacuInfo::print_bucket(vector<CacuRule*>& _rules) {
  FILE* fp = nullptr;
  fp = fopen("./INFO/DBT_buckets.txt", "a");
  if (fp == nullptr) {
    fprintf(stderr, "error - can not creat buckets.txt\n");
    return;
  }
  fprintf(fp,
          "Buckets Information [SIZE SIG]  (SIG={[0, 0], (0, 10], (10, 50], "
          "(50, 100], (100, +)})\n");
  fprintf(fp, "                  |- RULE\n");
  fprintf(fp, "                  |- ...\n");

  int bound_1 = 11;
  int bound_2 = 51;
  int bound_3 = 101;
  int small_bucket = 0;
  int mid_bucket = 0;
  int big_bucket = 0;
  const size_t rulesNum = _rules.size();
  for (size_t i = 0; i < rulesNum; ++i) {
    if (_rules[i]->is_first) {
      int _bucket_size = _rules[i]->size;
      fprintf(fp, "\nSIZE= %d ", _bucket_size);
      if (_bucket_size < bound_1) {
        fprintf(fp, "(0, 10]\n");
      } else if (_bucket_size < bound_2) {
        fprintf(fp, "(10, 50]\n");
        ++small_bucket;
      } else if (_bucket_size < bound_3) {
        fprintf(fp, "(50, 100]\n");
        ++mid_bucket;
      } else {
        fprintf(fp, "(100, +)\n");
        ++big_bucket;
      }
      for (int j = 0; j < _bucket_size; ++j) {
        fprintf(fp, "|- %u\t%u.%u.%u.%u/%u\t%u.%u.%u.%u/%u\t%u:%u\t%u:%u\t%d\n",
                _rules[i + j]->pri, _rules[i + j]->ip.i_8.sip[3],
                _rules[i + j]->ip.i_8.sip[2], _rules[i + j]->ip.i_8.sip[1],
                _rules[i + j]->ip.i_8.sip[0], _rules[i + j]->sip_length,
                _rules[i + j]->ip.i_8.dip[3], _rules[i + j]->ip.i_8.dip[2],
                _rules[i + j]->ip.i_8.dip[1], _rules[i + j]->ip.i_8.dip[0],
                _rules[i + j]->dip_length, _rules[i + j]->Port[0][0],
                _rules[i + j]->Port[0][1], _rules[i + j]->Port[1][0],
                _rules[i + j]->Port[1][1], _rules[i + j]->fetch_bit);
      }
    }
  }
  fclose(fp);

  std::cout << "buckets        : " << bucket_num << "\n";
  std::cout << "max bucket size: " << max_bucket_size << "\n";

  std::cout << "target buckets : " << target_bucket_num << " " << std::fixed
            << std::setprecision(2)
            << (static_cast<double>(target_bucket_num) / bucket_num * 100)
            << "%" << "\n";

  std::cout << "(10,50]        : " << small_bucket << " " << std::fixed
            << std::setprecision(2)
            << (static_cast<double>(small_bucket) / bucket_num * 100) << "%"
            << "\n";

  std::cout << "(50,100]       : " << mid_bucket << " " << std::fixed
            << std::setprecision(2)
            << (static_cast<double>(mid_bucket) / bucket_num * 100) << "%"
            << "\n";

  std::cout << "big cell       : " << big_bucket << " " << std::fixed
            << std::setprecision(2)
            << (static_cast<double>(big_bucket) / bucket_num * 100) << "%"
            << "\n";
}
///// ===== /////

//////// ======== ////////
DBTable::DBTable(vector<Rule>& _rules, int _threshold) : threshold(_threshold) {
  for (auto& _r : _rules) {
    ruleset.emplace_back(&_r);
  }
}

DBTable::DBTable(vector<Rule*>& _rules, int _threshold)
    : ruleset(_rules), threshold(_threshold) {}

DBTable::~DBTable() {
  for (int i = 0; i < subsets.size; ++i) {
    for (auto& _tuple : subsets.ipNodes[i].tuples) {
      for (int j = 0; j < _tuple.mask + 1; ++j) {
        // delete ptuple
        if (_tuple.ptuples[j].next != nullptr) {
          prefix_tuple* _ptuple = _tuple.ptuples[j].next;
          while (_ptuple != nullptr) {
            // delete port node
            for (int k = 0; k < 2; ++k) {
              if (_ptuple->pNodes[k] != nullptr) {
                bitset<16> bits = _ptuple->pNodes[k]->mask;
                delete[] _ptuple->pNodes[k]->buckets;
              }
            }
            prefix_tuple* del_ptuple = _ptuple;
            _ptuple = _ptuple->next;
            delete del_ptuple;
          }
        }
      }
      delete[] _tuple.ptuples;
    }
    if (subsets.ipNodes[i].prefix_down != nullptr) {
      for (int j = 0; j < 33; ++j) {
        for (int k = 0; k < 33; ++k) {
          delete[] subsets.ipNodes[i].prefix_down[j][k];
        }
        delete[] subsets.ipNodes[i].prefix_down[j];
      }
      delete[] subsets.ipNodes[i].prefix_down;
    }
  }
  delete[] subsets.ipNodes;
}

void DBTable::construct() {
  CacuInfo cacu(ruleset, threshold);
  cacu.CacuIpMask(subsets.mask);
  uint32_t bucket_num = 0;
  bitset<64> bits;
  bits = subsets.mask.i_64;
  // std::cout << "DBTable.cpp bits: " << bits
  //           << "\n";  // 印出整個 64 位元序列
  bucket_num = (1 << bits.count()) + 1;
  subsets.size = bucket_num;
  subsets.ipNodes = new ip_node[bucket_num]();
  subsets.nodes_num = bucket_num;

  for (auto _r : ruleset) insert_to_ipNode(_r);
  for (int j = 0; j < bucket_num; ++j) {
    if (subsets.ipNodes[j].pri != 0xFFFFFFFF &&
        subsets.ipNodes[j].rules.size() > C_BOUND)
      adjust_ipNode(&subsets.ipNodes[j]);
  }
  // print_nodes();
}

void DBTable::insert_to_ipNode(Rule* _r) {
  MASK _mask = {maskBit[_r->sip_length], maskBit[_r->dip_length]};
  uint32_t idx = (_mask.i_64 & subsets.mask.i_64) == subsets.mask.i_64
                     ? _pext_u64(_r->ip.i_64 & _mask.i_64, subsets.mask.i_64)
                     : (subsets.size - 1);
  if (subsets.ipNodes[idx].pri > _r->pri) subsets.ipNodes[idx].pri = _r->pri;
  subsets.ipNodes[idx].rules.emplace_back(*_r);
  // std::cout << subsets.ipNodes[idx].rules[0].protocol << "\n";
}

void DBTable::adjust_ipNode(ip_node* _node) {
  vector<Rule*> _rules;
  vector<TupleRange> tuple_ranges;
  double dt_time = 0;

  for (int i = 0; i < _node->rules.size(); ++i)
    _rules.emplace_back(&_node->rules[i]);
  tuple_ranges = DynamicTupleRanges(_rules, dt_time, tuple_ranges);
  const size_t tuple_ranges_num = tuple_ranges.size();

  _node->prefix_down = new char**[33]();
  for (int i = 0; i < 33; ++i) {
    _node->prefix_down[i] = new char*[33]();
    for (int j = 0; j < 33; ++j) _node->prefix_down[i][j] = new char[2]();
  }

  for (int i = 0; i < tuple_ranges_num; ++i)
    for (int x = tuple_ranges[i].x1; x <= tuple_ranges[i].x2; ++x)
      for (int y = tuple_ranges[i].y1; y <= tuple_ranges[i].y2; ++y) {
        _node->prefix_down[x][y][0] = tuple_ranges[i].x1;
        _node->prefix_down[x][y][1] = tuple_ranges[i].y1;
      }

  vector<MASK> p_length;

  // adjust rules
  // for (auto& _r : _node->rules) {
  for (int rid = 0; rid < _node->rules.size(); ++rid) {
    auto& _r = _node->rules[rid];
    if (_r.pri == 79960) {
      std::cout << ("\n");
    }
    MASK key = {maskBit[_node->prefix_down[_r.sip_length][_r.dip_length][0]],
                maskBit[_node->prefix_down[_r.sip_length][_r.dip_length][1]]};
    int i = 0;
    // find tuple
    for (; i < p_length.size(); ++i)
      if (key.i_64 == p_length[i].i_64) break;
    // creat tuple
    if (i == p_length.size()) {
      p_length.emplace_back(key);
      _node->tuples.emplace_back(Tuple(key));
      _node->tuples[i].pri = _r.pri;
      _node->tuples[i].ptuples = new prefix_tuple[8]();
    }
    Tuple* _tuple = &_node->tuples[i];
    // expansion hashtable
    if (_tuple->ptuple_num >= 0.75 * (_tuple->mask + 1)) {
      uint32_t new_mask = (_tuple->mask + 1) << 1;
      prefix_tuple* new_pTuples = new prefix_tuple[new_mask]();
      --new_mask;
      for (int j = 0; j < _tuple->mask + 1; ++j) {
        prefix_tuple* _tmp = &_tuple->ptuples[j];
        if (_tmp->pri != 0xFFFFFFFF) {
          uint32_t new_hash_id =
              hashCode(_tmp->prefix.i_32.sip, _tmp->prefix.i_32.dip) & new_mask;
          if (new_pTuples[new_hash_id].pri == 0xFFFFFFFF) {
            new_pTuples[new_hash_id] = *_tmp;
            //_tmp = _tmp->next;
            new_pTuples[new_hash_id].next = nullptr;
          } else {
            prefix_tuple* iter1 = &new_pTuples[new_hash_id];
            prefix_tuple* iter2 = &new_pTuples[new_hash_id];
            while (iter2 != nullptr) {
              if (_tmp->pri < iter2->pri) break;
              iter1 = iter2;
              iter2 = iter2->next;
            }
            prefix_tuple* _tmp1 = _tmp;
            //_tmp = _tmp->next;
            if (iter1 == iter2) {
              prefix_tuple* _tcopy = new prefix_tuple();
              *_tcopy = new_pTuples[new_hash_id];
              new_pTuples[new_hash_id] = *_tmp1;
              new_pTuples[new_hash_id].next = _tcopy;
            } else {
              prefix_tuple* _tcopy = new prefix_tuple();
              *_tcopy = *_tmp1;
              iter1->next = _tcopy;
              _tcopy->next = iter2;
            }
          }
        }
        _tmp = _tmp->next;
        while (_tmp != nullptr) {
          uint32_t new_hash_id =
              hashCode(_tmp->prefix.i_32.sip, _tmp->prefix.i_32.dip) & new_mask;
          if (new_pTuples[new_hash_id].pri == 0xFFFFFFFF) {
            new_pTuples[new_hash_id] = *_tmp;
            new_pTuples[new_hash_id].next = nullptr;
            prefix_tuple* del_pt = _tmp;
            _tmp = _tmp->next;
            delete del_pt;
          } else {
            prefix_tuple* iter1 = &new_pTuples[new_hash_id];
            prefix_tuple* iter2 = &new_pTuples[new_hash_id];
            while (iter2 != nullptr) {
              if (_tmp->pri < iter2->pri) break;
              iter1 = iter2;
              iter2 = iter2->next;
            }
            prefix_tuple* _tmp1 = _tmp;
            _tmp = _tmp->next;
            if (iter1 == iter2) {
              prefix_tuple _tcopy = *_tmp1;
              *_tmp1 = new_pTuples[new_hash_id];
              new_pTuples[new_hash_id] = _tcopy;
              new_pTuples[new_hash_id].next = _tmp1;
            } else {
              iter1->next = _tmp1;
              _tmp1->next = iter2;
            }
          }
        }
      }
      delete[] _tuple->ptuples;
      _tuple->ptuples = new_pTuples;
      _tuple->mask = new_mask;
    }
    // insert
    IP p_key = {_r.ip.i_64 & key.i_64};
    uint32_t idx = hashCode(p_key.i_32.sip, p_key.i_32.dip) & _tuple->mask;
    if (_tuple->ptuples[idx].pri == 0xFFFFFFFF) {
      _tuple->ptuples[idx].pri = _r.pri;
      _tuple->ptuples[idx].prefix = p_key;
      _tuple->ptuples[idx].rules.emplace_back(_r);
      ++_tuple->ptuple_num;
    } else {
      prefix_tuple* _pTuple = &_tuple->ptuples[idx];
      prefix_tuple* prior;
      while (_pTuple != nullptr) {
        if (_pTuple->prefix.i_64 == p_key.i_64) {
          _pTuple->rules.emplace_back(_r);
          break;
        }
        prior = _pTuple;
        _pTuple = _pTuple->next;
      }
      if (_pTuple == nullptr) {
        prefix_tuple* new_ptuple = new prefix_tuple();
        new_ptuple->pri = _r.pri;
        new_ptuple->prefix = p_key;
        new_ptuple->rules.emplace_back(_r);
        prior->next = new_ptuple;
        ++_tuple->ptuple_num;
      }
    }
  }
  _node->rules.free();

  // adjust ptuple
  for (auto& _tuple : _node->tuples) {
    for (int i = 0; i < _tuple.mask + 1; ++i) {
      prefix_tuple* _ptuple = &_tuple.ptuples[i];
      while (_ptuple != nullptr && _ptuple->pri != 0xFFFFFFFF) {
        if (_ptuple->rules.size() > 8) adjust_ptuple(_ptuple);
        _ptuple = _ptuple->next;
      }
    }
  }
}

void DBTable::adjust_ptuple(prefix_tuple* _ptuple) {
  vector<Rule*> _ruleset[2];
  myVector<Rule> _rules;
  for (int i = 0; i < _ptuple->rules.size(); ++i) {
    auto& _r = _ptuple->rules[i];
    if (_r.Port[1][0] == _r.Port[1][1])
      _ruleset[1].emplace_back(&_r);
    else if (_r.Port[0][0] == _r.Port[0][1])
      _ruleset[0].emplace_back(&_r);
    else
      _rules.emplace_back(_r);
  }

  for (int i = 0; i < 2; ++i) {
    if (_ruleset[i].size() != 0) {
      _ptuple->pNodes[i] = new port_node();
      _ptuple->pNodes[i]->pri = _ruleset[i][0]->pri;
      _ptuple->pNodes[i]->type = i;
      CacuInfo cacu(_ruleset[i], 4);
      _ptuple->pNodes[i]->mask = cacu.CacuPortMask(i);
      bitset<16> bits = _ptuple->pNodes[i]->mask;
      _ptuple->pNodes[i]->buckets = new Bucket[(1 << bits.count())]();
      for (auto _r : _ruleset[i]) {
        int idx = _pext_u32(_r->Port[i][0], _ptuple->pNodes[i]->mask);
        if (_ptuple->pNodes[i]->buckets[idx].pri > _r->pri) {
          _ptuple->pNodes[i]->buckets[idx].pri = _r->pri;
        }
        _ptuple->pNodes[i]->buckets[idx].rules.emplace_back(*_r);
      }
    }
  }

  _ptuple->rules.swap(_rules);
}

uint32_t DBTable::search(const Packet& _p) {
  uint32_t res = 0xFFFFFFFF;
  uint32_t bucket_id[2] = {_pext_u64(_p.ip.i_64, subsets.mask.i_64),
                           subsets.nodes_num - 1};
  // search in subset
  for (int j = 0; j < 2; ++j) {
    if (res > subsets.ipNodes[bucket_id[j]].pri) {
      // search in first layer
      if (!subsets.ipNodes[bucket_id[j]].rules.empty()) {
        // for (auto& _r : subsets.ipNodes[bucket_id[j]].rules) {
        for (int rid = 0; rid < subsets.ipNodes[bucket_id[j]].rules.size();
             ++rid) {
          auto& _r = subsets.ipNodes[bucket_id[j]].rules[rid];
          if (res < _r.pri) break;
          MASK _mask = {maskBit[_r.sip_length], maskBit[_r.dip_length]};
          if ((_r.ip.i_64 ^ _p.ip.i_64 & _r.mask.i_64) == 0 &&
              (_r.protocol.val & _r.protocol.mask) ==
                  (_p.protocol & _r.protocol.mask) &&
              _r.Port[0][0] <= _p.Port[0] && _r.Port[0][1] >= _p.Port[0] &&
              _r.Port[1][0] <= _p.Port[1] && _r.Port[1][1] >= _p.Port[1]) {
            res = _r.pri;
            break;
          }
        }
      }
      // else {
      //  search in tuple
      for (auto& _tuple : subsets.ipNodes[bucket_id[j]].tuples) {
        if (_tuple.pri > res) break;
        IP _prefix = {_p.ip.i_64 & _tuple.key.i_64};
        uint32_t hash1 = _prefix.i_32.sip;
        uint32_t hash2 = _prefix.i_32.dip;
        hash1 ^= hash1 >> 16;
        hash1 *= 0x85ebca6b;
        hash1 ^= hash1 >> 13;
        hash1 *= 0xc2b2ae35;
        hash2 ^= hash2 >> 16;
        hash2 *= 0x85ebca6b;
        hash2 ^= hash2 >> 13;
        hash2 *= 0xc2b2ae35;
        hash1 ^= hash2;
        hash1 ^= hash1 >> 16;
        prefix_tuple* _ptuple = &_tuple.ptuples[hash1 & _tuple.mask];
        while (_ptuple != nullptr) {
          if (_ptuple->pri > res) break;
          // search in ptuple
          if (_prefix.i_64 == _ptuple->prefix.i_64) {
            // search in second layer
            if (!_ptuple->rules.empty()) {
              // for (auto& _r : _ptuple->rules) {
              for (int rid = 0; rid < _ptuple->rules.size(); ++rid) {
                auto& _r = _ptuple->rules[rid];
                if (res < _r.pri) break;
                if ((_r.ip.i_64 ^ _p.ip.i_64 & _r.mask.i_64) == 0 &&
                    (_r.protocol.val & _r.protocol.mask) ==
                        (_p.protocol & _r.protocol.mask) &&
                    _r.Port[0][0] <= _p.Port[0] &&
                    _r.Port[0][1] >= _p.Port[0] &&
                    _r.Port[1][0] <= _p.Port[1] &&
                    _r.Port[1][1] >= _p.Port[1]) {
                  res = _r.pri;
                  break;
                }
              }
            }
            // search in port_node
            for (int k = 0; k < 2; ++k) {
              if (_ptuple->pNodes[k] == nullptr ||
                  _ptuple->pNodes[k]->pri > res)
                continue;
              uint32_t pn_id = _pext_u32(_p.Port[_ptuple->pNodes[k]->type],
                                         _ptuple->pNodes[k]->mask);
              if (res > _ptuple->pNodes[k]->buckets[pn_id].pri) {
                // for (auto& _r : _ptuple->pNodes[k]->buckets[pn_id].rules) {
                for (int rid = 0;
                     rid < _ptuple->pNodes[k]->buckets[pn_id].rules.size();
                     ++rid) {
                  auto& _r = _ptuple->pNodes[k]->buckets[pn_id].rules[rid];
                  if (res < _r.pri) break;
                  if ((_r.ip.i_64 ^ _p.ip.i_64 & _r.mask.i_64) == 0 &&
                      (_r.protocol.val & _r.protocol.mask) ==
                          (_p.protocol & _r.protocol.mask) &&
                      _r.Port[0][0] <= _p.Port[0] &&
                      _r.Port[0][1] >= _p.Port[0] &&
                      _r.Port[1][0] <= _p.Port[1] &&
                      _r.Port[1][1] >= _p.Port[1]) {
                    res = _r.pri;
                    break;
                  }
                }
              }
            }
            break;
          }
          _ptuple = _ptuple->next;
        }
      }
      //}
    }
  }
  return res;
}

void DBTable::search_with_log(const vector<Packet>& _packet) {
  uint64_t acc_bucket = 0;
  uint64_t acc_tuple = 0;
  uint64_t acc_rule = 0;
  int max_bucket = 0;
  int max_tuple = 0;
  int max_rule = 0;
  for (auto& _p : _packet) {
    int acc_b, acc_t, acc_r;
    acc_b = acc_t = acc_r = 0;
    uint32_t res = 0xFFFFFFFF;
    uint32_t bucket_id[2] = {_pext_u64(_p.ip.i_64, subsets.mask.i_64),
                             subsets.nodes_num - 1};
    // search in subset
    for (int j = 0; j < 2; ++j) {
      if (res > subsets.ipNodes[bucket_id[j]].pri) {
        ++acc_b;
        // search in first layer
        if (!subsets.ipNodes[bucket_id[j]].rules.empty()) {
          // for (auto& _r : subsets.ipNodes[bucket_id[j]].rules) {
          for (int rid = 0; rid < subsets.ipNodes[bucket_id[j]].rules.size();
               ++rid) {
            ++acc_r;
            auto& _r = subsets.ipNodes[bucket_id[j]].rules[rid];
            if (res < _r.pri) break;
            MASK _mask = {maskBit[_r.sip_length], maskBit[_r.dip_length]};
            if ((_r.ip.i_64 ^ _p.ip.i_64 & _r.mask.i_64) == 0 &&
                (_r.protocol.val & _r.protocol.mask) ==
                    (_p.protocol & _r.protocol.mask) &&
                _r.Port[0][0] <= _p.Port[0] && _r.Port[0][1] >= _p.Port[0] &&
                _r.Port[1][0] <= _p.Port[1] && _r.Port[1][1] >= _p.Port[1]) {
              res = _r.pri;
              break;
            }
          }
        } else {
          // search in tuple
          for (auto& _tuple : subsets.ipNodes[bucket_id[j]].tuples) {
            if (_tuple.pri > res) break;
            ++acc_t;
            IP _prefix = {_p.ip.i_64 & _tuple.key.i_64};
            uint32_t hash1 = _prefix.i_32.sip;
            uint32_t hash2 = _prefix.i_32.dip;
            hash1 ^= hash1 >> 16;
            hash1 *= 0x85ebca6b;
            hash1 ^= hash1 >> 13;
            hash1 *= 0xc2b2ae35;
            hash2 ^= hash2 >> 16;
            hash2 *= 0x85ebca6b;
            hash2 ^= hash2 >> 13;
            hash2 *= 0xc2b2ae35;
            hash1 ^= hash2;
            hash1 ^= hash1 >> 16;
            prefix_tuple* _ptuple = &_tuple.ptuples[hash1 & _tuple.mask];
            while (_ptuple != nullptr) {
              if (_ptuple->pri > res) break;
              // search in ptuple
              if (_prefix.i_64 == _ptuple->prefix.i_64) {
                // search in second layer
                if (!_ptuple->rules.empty()) {
                  // for (auto& _r : _ptuple->rules) {
                  for (int rid = 0; rid < _ptuple->rules.size(); ++rid) {
                    ++acc_r;
                    auto& _r = _ptuple->rules[rid];
                    if (res < _r.pri) break;
                    if ((_r.ip.i_64 ^ _p.ip.i_64 & _r.mask.i_64) == 0 &&
                        (_r.protocol.val & _r.protocol.mask) ==
                            (_p.protocol & _r.protocol.mask) &&
                        _r.Port[0][0] <= _p.Port[0] &&
                        _r.Port[0][1] >= _p.Port[0] &&
                        _r.Port[1][0] <= _p.Port[1] &&
                        _r.Port[1][1] >= _p.Port[1]) {
                      res = _r.pri;
                      break;
                    }
                  }
                }
                // search in port_node
                for (int k = 0; k < 2; ++k) {
                  if (_ptuple->pNodes[k] == nullptr ||
                      _ptuple->pNodes[k]->pri > res)
                    continue;
                  uint32_t pn_id = _pext_u32(_p.Port[_ptuple->pNodes[k]->type],
                                             _ptuple->pNodes[k]->mask);
                  if (res > _ptuple->pNodes[k]->buckets[pn_id].pri) {
                    // for (auto& _r : _ptuple->pNodes[k]->buckets[pn_id].rules)
                    // {
                    for (int rid = 0;
                         rid < _ptuple->pNodes[k]->buckets[pn_id].rules.size();
                         ++rid) {
                      ++acc_r;
                      auto& _r = _ptuple->pNodes[k]->buckets[pn_id].rules[rid];
                      if (res < _r.pri) break;
                      MASK _mask = {maskBit[_r.sip_length],
                                    maskBit[_r.dip_length]};
                      if ((_r.ip.i_64 ^ _p.ip.i_64 & _r.mask.i_64) == 0 &&
                          (_r.protocol.val & _r.protocol.mask) ==
                              (_p.protocol & _r.protocol.mask) &&
                          _r.Port[0][0] <= _p.Port[0] &&
                          _r.Port[0][1] >= _p.Port[0] &&
                          _r.Port[1][0] <= _p.Port[1] &&
                          _r.Port[1][1] >= _p.Port[1]) {
                        res = _r.pri;
                        break;
                      }
                    }
                  }
                }
                break;
              }
              _ptuple = _ptuple->next;
            }
          }
        }
      }
    }
    acc_bucket += acc_b;
    acc_tuple += acc_t;
    acc_rule += acc_r;
    if (max_bucket < acc_b) max_bucket = acc_b;
    if (max_tuple < acc_t) max_tuple = acc_t;
    if (max_rule < acc_r) max_rule = acc_r;
  }
  const size_t packetNum = _packet.size();
  std::cout << "\navg_acc_bucket: " << (double)acc_bucket / (double)packetNum
            << " max: " << max_bucket << "\n";

  std::cout << "avg_acc_tuple: " << (double)acc_tuple / (double)packetNum
            << " max: " << max_tuple << "\n";

  std::cout << "avg_acc_rule: " << (double)acc_rule / (double)packetNum
            << " max: " << max_rule << "\n";
}

void DBTable::insert(Rule& _r) {
  MASK _mask = {maskBit[_r.sip_length], maskBit[_r.dip_length]};
  uint32_t idx = (_mask.i_64 & subsets.mask.i_64) == subsets.mask.i_64
                     ? _pext_u64(_r.ip.i_64 & _mask.i_64, subsets.mask.i_64)
                     : (subsets.size - 1);
  if (subsets.ipNodes[idx].pri > _r.pri) subsets.ipNodes[idx].pri = _r.pri;
  // insert to tuple
  if (!subsets.ipNodes[idx].tuples.empty()) {
    MASK key = {maskBit[subsets.ipNodes[idx]
                            .prefix_down[_r.sip_length][_r.dip_length][0]],
                maskBit[subsets.ipNodes[idx]
                            .prefix_down[_r.sip_length][_r.dip_length][1]]};
    int i = 0;
    for (; i < subsets.ipNodes[idx].tuples.size(); ++i) {
      if (subsets.ipNodes[idx].tuples[i].key.i_64 == key.i_64) {
        break;
      }
    }
    if (i != subsets.ipNodes[idx].tuples.size()) {
      // add new tuple

      Tuple* _tuple = &subsets.ipNodes[idx].tuples[i];

      // insert
      IP p_key = {_r.ip.i_64 & key.i_64};
      uint32_t hash_idx =
          hashCode(p_key.i_32.sip, p_key.i_32.dip) & _tuple->mask;
      if (_tuple->ptuples[hash_idx].pri == 0xFFFFFFFF) {
        _tuple->ptuples[hash_idx].pri = _r.pri;
        _tuple->ptuples[hash_idx].prefix = p_key;
        _tuple->ptuples[hash_idx].rules.emplace_back(_r);
        ++_tuple->ptuple_num;
      } else {
        prefix_tuple* _pTuple = &_tuple->ptuples[hash_idx];
        prefix_tuple* prior;
        while (_pTuple != nullptr) {
          if (_pTuple->prefix.i_64 == p_key.i_64) {
            // if have port_node
            if (_pTuple->pNodes[1] != nullptr &&
                _r.Port[1][0] == _r.Port[1][1]) {
              int b_idx = _pext_u32(_r.Port[1][0], _pTuple->pNodes[1]->mask);
              if (_pTuple->pNodes[1]->buckets[b_idx].pri > _r.pri) {
                _pTuple->pNodes[1]->buckets[b_idx].pri = _r.pri;
              }
              int pri_idx = 0;
              for (; pri_idx < _pTuple->pNodes[1]->buckets[b_idx].rules.size();
                   ++pri_idx) {
                if (_pTuple->pNodes[1]->buckets[b_idx].rules[pri_idx].pri >
                    _r.pri)
                  break;
              }
              _pTuple->pNodes[1]->buckets[b_idx].rules.insert(_r, pri_idx);
              return;
            } else if (_pTuple->pNodes[0] != nullptr &&
                       _r.Port[0][0] == _r.Port[0][1]) {
              int b_idx = _pext_u32(_r.Port[0][0], _pTuple->pNodes[0]->mask);
              if (_pTuple->pNodes[0]->buckets[b_idx].pri > _r.pri) {
                _pTuple->pNodes[0]->buckets[b_idx].pri = _r.pri;
              }
              int pri_idx = 0;
              for (; pri_idx < _pTuple->pNodes[0]->buckets[b_idx].rules.size();
                   ++pri_idx) {
                if (_pTuple->pNodes[0]->buckets[b_idx].rules[pri_idx].pri >
                    _r.pri)
                  break;
              }
              _pTuple->pNodes[0]->buckets[b_idx].rules.insert(_r, pri_idx);
              return;
            } else {
              int pri_idx = 0;
              for (; pri_idx < _pTuple->rules.size(); ++pri_idx) {
                if (_pTuple->rules[pri_idx].pri > _r.pri) break;
              }
              _pTuple->rules.insert(_r, pri_idx);
              return;
            }
          }
          prior = _pTuple;
          _pTuple = _pTuple->next;
        }
        if (_pTuple == nullptr) {
          prefix_tuple* new_ptuple = new prefix_tuple();
          new_ptuple->pri = _r.pri;
          new_ptuple->prefix = p_key;
          new_ptuple->rules.emplace_back(_r);
          prior->next = new_ptuple;
          ++_tuple->ptuple_num;
          return;
        }
      }
    } else {
      int pri_idx = 0;
      for (; pri_idx < subsets.ipNodes[idx].rules.size(); ++pri_idx) {
        if (subsets.ipNodes[idx].rules[pri_idx].pri > _r.pri) break;
      }
      subsets.ipNodes[idx].rules.insert(_r, pri_idx);
      return;
    }
  } else {
    int pri_idx = 0;
    for (; pri_idx < subsets.ipNodes[idx].rules.size(); ++pri_idx) {
      if (subsets.ipNodes[idx].rules[pri_idx].pri > _r.pri) break;
    }
    subsets.ipNodes[idx].rules.insert(_r, pri_idx);
    return;
  }
}

void DBTable::remove(Rule& _r) {
  MASK _mask = {maskBit[_r.sip_length], maskBit[_r.dip_length]};
  uint32_t idx = (_mask.i_64 & subsets.mask.i_64) == subsets.mask.i_64
                     ? _pext_u64(_r.ip.i_64 & _mask.i_64, subsets.mask.i_64)
                     : (subsets.size - 1);
  // search in tuple
  if (!subsets.ipNodes[idx].tuples.empty()) {
    MASK key = {maskBit[subsets.ipNodes[idx]
                            .prefix_down[_r.sip_length][_r.dip_length][0]],
                maskBit[subsets.ipNodes[idx]
                            .prefix_down[_r.sip_length][_r.dip_length][1]]};
    int i = 0;
    for (; i < subsets.ipNodes[idx].tuples.size(); ++i) {
      if (subsets.ipNodes[idx].tuples[i].key.i_64 == key.i_64) {
        break;
      }
    }
    if (i == subsets.ipNodes[idx].tuples.size()) {
      int pri_idx = 0;
      for (; pri_idx < subsets.ipNodes[idx].rules.size(); ++pri_idx) {
        if (subsets.ipNodes[idx].rules[pri_idx].pri == _r.pri) break;
      }
      if (pri_idx == subsets.ipNodes[idx].rules.size()) {
        std::cerr << ("err-can not find rule in bucket.\n");
        return;
      }
      subsets.ipNodes[idx].rules.remove(pri_idx);
      return;
    } else {
      Tuple* _tuple = &subsets.ipNodes[idx].tuples[i];
      // search
      IP p_key = {_r.ip.i_64 & key.i_64};
      uint32_t hash_idx =
          hashCode(p_key.i_32.sip, p_key.i_32.dip) & _tuple->mask;
      if (_tuple->ptuples[hash_idx].pri == 0xFFFFFFFF) {
        std::cerr << ("err-can not find ptuple.\n");
        return;
      } else {
        prefix_tuple* _pTuple = &_tuple->ptuples[hash_idx];
        prefix_tuple* prior;
        while (_pTuple != nullptr) {
          if (_pTuple->prefix.i_64 == p_key.i_64) {
            // if have port_node
            if (_pTuple->pNodes[1] != nullptr &&
                _r.Port[1][0] == _r.Port[1][1]) {
              int b_idx = _pext_u32(_r.Port[1][0], _pTuple->pNodes[1]->mask);
              int pri_idx = 0;
              for (; pri_idx < _pTuple->pNodes[1]->buckets[b_idx].rules.size();
                   ++pri_idx) {
                if (_pTuple->pNodes[1]->buckets[b_idx].rules[pri_idx].pri ==
                    _r.pri)
                  break;
              }
              if (pri_idx == _pTuple->pNodes[1]->buckets[b_idx].rules.size()) {
                std::cerr << ("err-can not find rule in dport.\n");
                return;
              }
              _pTuple->pNodes[1]->buckets[b_idx].rules.remove(pri_idx);
              return;
            } else if (_pTuple->pNodes[0] != nullptr &&
                       _r.Port[0][0] == _r.Port[0][1]) {
              int b_idx = _pext_u32(_r.Port[0][0], _pTuple->pNodes[0]->mask);
              int pri_idx = 0;
              for (; pri_idx < _pTuple->pNodes[0]->buckets[b_idx].rules.size();
                   ++pri_idx) {
                if (_pTuple->pNodes[0]->buckets[b_idx].rules[pri_idx].pri ==
                    _r.pri)
                  break;
              }
              if (pri_idx == _pTuple->pNodes[0]->buckets[b_idx].rules.size()) {
                std::cerr << ("err-can not find rule in sport.\n");
                return;
              }
              _pTuple->pNodes[0]->buckets[b_idx].rules.remove(pri_idx);
              return;
            } else {
              int pri_idx = 0;
              for (; pri_idx < _pTuple->rules.size(); ++pri_idx) {
                if (_pTuple->rules[pri_idx].pri == _r.pri) break;
              }
              if (pri_idx == _pTuple->rules.size()) {
                std::cerr << ("err-can not find rule in ptuple.\n");
                return;
              }
              _pTuple->rules.remove(pri_idx);
              return;
            }
          }
          prior = _pTuple;
          _pTuple = _pTuple->next;
        }
        if (_pTuple == nullptr) {
          std::cerr << ("err-can not find ptuple-2.\n");
          return;
        }
      }
    }
  } else {
    int pri_idx = 0;
    for (; pri_idx < subsets.ipNodes[idx].rules.size(); ++pri_idx) {
      if (subsets.ipNodes[idx].rules[pri_idx].pri == _r.pri) break;
    }
    if (pri_idx == subsets.ipNodes[idx].rules.size()) {
      std::cerr << ("err-can not find rule in bucket.\n");
      return;
    }
    subsets.ipNodes[idx].rules.remove(pri_idx);
  }
}

void DBTable::print_nodes() {
  FILE* fp = nullptr;
  fp = fopen("./INFO/DBT_nodes.txt", "w");
  if (fp == nullptr) {
    fprintf(stderr, "error - can not creat nodes.txt\n");
    return;
  }
  fprintf(fp,
          "Nodes Information [SID SIZE SIG PN_SIZE PT_SIZE]  (SIG={[0, 0], (0, "
          "10], (10, 50], (50, 100], (100, +)})\n");
  fprintf(fp, "                  |- RULE\n");
  fprintf(fp, "                  |- ...\n");

  int bound_1 = 11;
  int bound_2 = 51;
  int bound_3 = 101;
  bool have_false = false;

  int target_bucket_num = 0;
  int max_bucket_size = 0;
  int small_bucket = 0;
  int mid_bucket = 0;
  int big_bucket = 0;
  int total_bucket_num = 0;
  int used_bucket_num = 0;
  int used_tuple_space = 0;
  int used_tuple = 0;
  int max_tuples = 0;
  uint32_t rule_num = 0;
  uint32_t rule_num_in_bucket = 0;

  total_bucket_num = subsets.nodes_num;
  for (size_t i = 0; i < subsets.nodes_num; ++i) {
    if (subsets.ipNodes[i].pri != 0xFFFFFFFF) {
      ++used_bucket_num;
      int _bucket_size;
      if (!subsets.ipNodes[i].rules.empty()) {
        _bucket_size = subsets.ipNodes[i].rules.size();
        fprintf(fp, "\nSIZE= %d ", _bucket_size);
        if (_bucket_size < threshold) ++target_bucket_num;
        if (_bucket_size > max_bucket_size) max_bucket_size = _bucket_size;
        if (_bucket_size < bound_1) {
          fprintf(fp, "(0, 10]\n");
        } else if (_bucket_size < bound_2) {
          fprintf(fp, "(10, 50]\n");
          ++small_bucket;
        } else if (_bucket_size < bound_3) {
          fprintf(fp, "(50, 100]\n");
          ++mid_bucket;
        } else {
          fprintf(fp, "(100, +)\n");
          ++big_bucket;
        }
        rule_num += subsets.ipNodes[i].rules.size();
        rule_num_in_bucket += subsets.ipNodes[i].rules.size();
        // for (auto& _r : subsets.ipNodes[i].rules) {
        for (int rid = 0; rid < subsets.ipNodes[i].rules.size(); ++rid) {
          auto& _r = subsets.ipNodes[i].rules[rid];
          fprintf(
              fp,
              "|- "
              "%u\t%u.%u.%u.%u/%u\t\t%u.%u.%u.%u/%u\t\t%u:%u\t\t%u:%u\t\t%u\n",
              _r.pri, _r.ip.i_8.sip[3], _r.ip.i_8.sip[2], _r.ip.i_8.sip[1],
              _r.ip.i_8.sip[0], _r.sip_length, _r.ip.i_8.dip[3],
              _r.ip.i_8.dip[2], _r.ip.i_8.dip[1], _r.ip.i_8.dip[0],
              _r.dip_length, _r.Port[0][0], _r.Port[0][1], _r.Port[1][0],
              _r.Port[1][1], _r.protocol.val);
        }
      }
      if (!subsets.ipNodes[i].tuples.empty()) {
        ++used_tuple_space;
        used_tuple += subsets.ipNodes[i].tuples.size();
        if (subsets.ipNodes[i].tuples.size() > max_tuples)
          max_tuples = subsets.ipNodes[i].tuples.size();
        fprintf(fp, "\nUSED_TUPLES %zu", subsets.ipNodes[i].tuples.size());
        for (auto& _tuple : subsets.ipNodes[i].tuples) {
          total_bucket_num += _tuple.mask + 1;
          used_bucket_num += _tuple.ptuple_num;
          for (int j = 0; j < _tuple.mask + 1; ++j) {
            prefix_tuple* _ptuple = &_tuple.ptuples[j];
            while (_ptuple != nullptr && _ptuple->pri != 0xFFFFFFFF) {
              if (!_ptuple->rules.empty()) {
                _bucket_size = _ptuple->rules.size();
                bitset<32> bits;
                bits = _tuple.key.i_32.smask;
                int tu_1 = bits.count();
                bits = _tuple.key.i_32.dmask;
                int tu_2 = bits.count();
                fprintf(fp, "\nPT_SIZE= %d TUPLE (%d,%d) HASH %d ",
                        _bucket_size, tu_1, tu_2, j);
                if (_bucket_size < threshold) ++target_bucket_num;
                if (_bucket_size > max_bucket_size)
                  max_bucket_size = _bucket_size;
                if (_bucket_size < bound_1) {
                  fprintf(fp, "(0, 10]\n");
                } else if (_bucket_size < bound_2) {
                  fprintf(fp, "(10, 50]\n");
                  ++small_bucket;
                } else if (_bucket_size < bound_3) {
                  fprintf(fp, "(50, 100]\n");
                  ++mid_bucket;
                } else {
                  fprintf(fp, "(100, +)\n");
                  ++big_bucket;
                }
                rule_num += _ptuple->rules.size();
                // for (auto& _r : _ptuple->rules) {
                for (int rid = 0; rid < _ptuple->rules.size(); ++rid) {
                  auto& _r = _ptuple->rules[rid];
                  fprintf(fp,
                          "|- "
                          "%u\t%u.%u.%u.%u/%u\t\t%u.%u.%u.%u/"
                          "%u\t\t%u:%u\t\t%u:%u\t\t%u\n",
                          _r.pri, _r.ip.i_8.sip[3], _r.ip.i_8.sip[2],
                          _r.ip.i_8.sip[1], _r.ip.i_8.sip[0], _r.sip_length,
                          _r.ip.i_8.dip[3], _r.ip.i_8.dip[2], _r.ip.i_8.dip[1],
                          _r.ip.i_8.dip[0], _r.dip_length, _r.Port[0][0],
                          _r.Port[0][1], _r.Port[1][0], _r.Port[1][1],
                          _r.protocol.val);
                }
              }
              for (int k = 0; k < 2; ++k) {
                if (_ptuple->pNodes[k] != nullptr) {
                  port_node* p_node = _ptuple->pNodes[k];
                  bitset<16> bits = p_node->mask;
                  int pn_size = 1 << bits.count();
                  total_bucket_num += pn_size;
                  for (int v = 0; v < pn_size; ++v) {
                    if (p_node->buckets[v].pri != 0xFFFFFFFF) {
                      ++used_bucket_num;
                      _bucket_size = p_node->buckets[v].rules.size();
                      fprintf(fp, "\nPN_SIZE= %d ", _bucket_size);
                      if (_bucket_size < threshold) ++target_bucket_num;
                      if (_bucket_size > max_bucket_size)
                        max_bucket_size = _bucket_size;
                      if (_bucket_size < bound_1) {
                        fprintf(fp, "(0, 10]\n");
                      } else if (_bucket_size < bound_2) {
                        fprintf(fp, "(10, 50]\n");
                        ++small_bucket;
                      } else if (_bucket_size < bound_3) {
                        fprintf(fp, "(50, 100]\n");
                        ++mid_bucket;
                      } else {
                        fprintf(fp, "(100, +)\n");
                        ++big_bucket;
                      }
                      rule_num += p_node->buckets[v].rules.size();
                      // for (auto& _r : p_node->buckets[v].rules) {
                      for (int rid = 0; rid < p_node->buckets[v].rules.size();
                           ++rid) {
                        auto& _r = p_node->buckets[v].rules[rid];
                        fprintf(fp,
                                "|- "
                                "%u\t%u.%u.%u.%u/%u\t\t%u.%u.%u.%u/"
                                "%u\t\t%u:%u\t\t%u:%u\t\t%u\n",
                                _r.pri, _r.ip.i_8.sip[3], _r.ip.i_8.sip[2],
                                _r.ip.i_8.sip[1], _r.ip.i_8.sip[0],
                                _r.sip_length, _r.ip.i_8.dip[3],
                                _r.ip.i_8.dip[2], _r.ip.i_8.dip[1],
                                _r.ip.i_8.dip[0], _r.dip_length, _r.Port[0][0],
                                _r.Port[0][1], _r.Port[1][0], _r.Port[1][1],
                                _r.protocol.val);
                      }
                    }
                  }
                }
              }
              _ptuple = _ptuple->next;
            }
          }
        }
      }
    }
  }
  // std::cout << "rule_num " << rule_num << " " << ruleset.size() << "\n";

  std::cout << "in_bucket " << rule_num_in_bucket << " "
            << (double)rule_num_in_bucket / ruleset.size() << "\n";

  std::cout << "in_tuple " << (rule_num - rule_num_in_bucket) << " "
            << (double)(rule_num - rule_num_in_bucket) / ruleset.size() << "\n";

  std::cout << "total buckets  : " << total_bucket_num << "\n";

  std::cout << "used buckets   : " << used_bucket_num << " "
            << (double)used_bucket_num / (double)total_bucket_num * 100 << "%"
            << "\n";

  std::cout << "max bucket size: " << max_bucket_size << "\n";

  std::cout << "target buckets : " << target_bucket_num << " "
            << (double)target_bucket_num / (double)used_bucket_num * 100 << "%"
            << "\n";

  std::cout << "(10,50]        : " << small_bucket << " "
            << (double)small_bucket / (double)used_bucket_num * 100 << "%"
            << "\n";

  std::cout << "(50,100]       : " << mid_bucket << " "
            << (double)mid_bucket / (double)used_bucket_num * 100 << "%"
            << "\n";

  std::cout << "big cell       : " << big_bucket << " "
            << (double)big_bucket / (double)used_bucket_num * 100 << "%"
            << "\n";

  std::cout << "tuple spaces   : " << used_tuple_space << "\n";

  std::cout << "avg tuples     : "
            << (double)used_tuple / (double)used_tuple_space << "\n";

  std::cout << "max tuples     : " << max_tuples << "\n" << "\n";
  fclose(fp);
}

void DBTable::mem() {
  size_t mem = sizeof(SubSet) + subsets.size * sizeof(ip_node);
  for (int i = 0; i < subsets.size; ++i) {
    if (subsets.ipNodes[i].pri != 0xFFFFFFFF) {
      // mem += (subsets.ipNodes[i].rules.size() * sizeof(Rule) +
      // subsets.ipNodes[i].tuples.size() * sizeof(Tuple));
      mem += (subsets.ipNodes[i].tuples.size() * sizeof(Tuple));
      for (auto& _tuple : subsets.ipNodes[i].tuples) {
        mem += tuple_mem(_tuple);
      }
    }
    if (subsets.ipNodes[i].prefix_down != nullptr) mem += (2178 * sizeof(char));
  }
  std::cout << "\nTotal memory " << (mem / 1048676.0) << " MB" << "\n";
}

size_t DBTable::tuple_mem(Tuple& _tuple) {
  size_t mem = 0;
  for (int i = 0; i < (_tuple.mask + 1); ++i) {
    if (_tuple.ptuples[i].pri != 0xFFFFFFFF) {
      mem += ptule_mem(_tuple.ptuples[i]);
    } else {
      mem += sizeof(prefix_tuple);
    }
  }
  return mem;
}

size_t DBTable::ptule_mem(prefix_tuple& _ptuple) {
  prefix_tuple* _p = &_ptuple;
  size_t mem = 0;
  while (_p != nullptr) {
    // mem += (sizeof(prefix_tuple) + _ptuple.rules.size() * sizeof(Rule));
    mem += (sizeof(prefix_tuple));
    for (int i = 0; i < 2; ++i) {
      if (_p->pNodes[i] != nullptr) mem += portNode_mem(_p->pNodes[i]);
    }
    _p = _p->next;
  }
  return mem;
}

size_t DBTable::portNode_mem(port_node* _pnode) {
  bitset<16> bits = _pnode->mask;
  int pn_size = 1 << bits.count();
  size_t mem = sizeof(port_node) + pn_size * sizeof(Bucket);
  /*for (int i = 0; i < pn_size; ++i) {
          mem += (_pnode->buckets[i].rules.size() * sizeof(Rule));
  }*/
  return mem;
}
//////// ======== ////////
}  // namespace DBT
