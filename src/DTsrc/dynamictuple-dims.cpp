#include "dynamictuple-dims.h"

using namespace std;

uint64_t dim_check_tuple_cost = 5;  // us
// uint64_t check_group_cost = 2;
uint64_t dim_check_rule_cost = 5;

int dim_prefix_len[5] = {32, 32, 16, 16, 8};

void DimsRange::Init() {
  int step = 8;
  for (int i = 0; i < 5; ++i) {
    dims[i].clear();
    for (int j = 0; j <= dim_prefix_len[i]; j += step) {
      int k = min(j + step, dim_prefix_len[i]);
      DimRange dim_range;
      dim_range.x1 = j;
      dim_range.x2 = k;
      dims[i].push_back(dim_range);
      for (int p = j; p <= k; ++p) reduced[i][p] = j;
    }
  }
}

bool CmpDimRulePriority(DimRule *rule1, DimRule *rule2) {
  return rule1->priority > rule2->priority;
}

int cmp_dim;

bool CmpDimRulePrefixLen(DimRule *rule1, DimRule *rule2) {
  return rule1->prefix_len[cmp_dim] < rule2->prefix_len[cmp_dim];
}

bool CmpDimRuleReducedKey(DimRule *rule1, DimRule *rule2) {
  for (int i = 0; i < 5; ++i)
    if (rule1->reduced_key[i] != rule2->reduced_key[i])
      return rule1->reduced_key[i] < rule2->reduced_key[i];
  return rule1->priority > rule2->priority;
}

bool SameReducedKey(DimRule *rule1, DimRule *rule2) {
  for (int i = 0; i < 5; ++i)
    if (rule1->reduced_key[i] != rule2->reduced_key[i]) return false;
  return true;
}

void DimInfo::GetDimRules(vector<Rule_DT_MT *> &_rules) {
  rules_num = _rules.size();
  // printf("rules_num %d\n", rules_num);
  for (int i = 0; i < rules_num; ++i) {
    DimRule *rule = (DimRule *)malloc(sizeof(DimRule));
    for (int j = 0; j < 5; ++j) {
      rule->key[j] = _rules[i]->range[j][0];
      rule->prefix_len[j] = _rules[i]->prefix_len[j];
      rule->priority = _rules[i]->priority;
    }
    rules.push_back(rule);
  }
  sort(rules.begin(), rules.end(), CmpDimRulePriority);
  for (int i = 0; i < rules_num; ++i) rules[i]->priority = rules_num - i;
}

uint32_t GetPrefixLen(DimRule *rule, DimsRange &dims_range, int except_dim) {
  uint32_t prefix_len = 0;
  for (int i = 0; i < 5; ++i) {
    prefix_len <<= 6;
    if (i != except_dim)
      prefix_len |= dims_range.reduced[i][rule->prefix_len[i]];
  }
  return prefix_len;
}

void PrintPrefixLen(uint32_t prefix_len) {
  char lens[5];
  for (int i = 4; i >= 0; --i) {
    lens[i] = prefix_len & 0x3F;
    prefix_len >>= 6;
  }
  for (const auto &len : lens) {
    std::cout << len << ' ';
  }
  std::cout << '\n';
}

DimKey GetReducedDimKey(DimRule *rule) {
  DimKey dim_key;
  for (int j = 0; j < 5; ++j) dim_key.key[j] = rule->reduced_key[j];
  return dim_key;
}

// void CalculateDimCost(vector<DimRule*> rules, DimsRange &dims_range, int dim)
// { 	int rules_num = rules.size(); 	sort(rules.begin(), rules.end(),
// CmpDimRulePriority);

// 	tuples_priority.clear();
// 	tuples_key[0].clear();
// 	uint64_t check_tuples_num = 0;
// 	uint64_t check_rules_num = 0;
// 	memset(dim_check_rules_num, 0, sizeof(dim_check_rules_num));
// 	for (int i = 0; i < rules_num; ++i)  {
// 		for (int j = 0; j < 5; ++j) {
// 			int k = dim_prefix_len[j] -
// dims_range.reduced[j][rules[i]->prefix_len[j]];
// rules[i]->reduced_key[j] = (uint64_t)rules[i]->key[j] >> k << k;
// 		}
// 		uint32_t prefix_len = GetPrefixLen(rules[i], dims_range, -1);
// 		DimKey dim_key = GetReducedDimKey(rules[i]);
// 		if (rules[i]->priority > tuples_priority[prefix_len]) {
// 			check_tuples_num += rules[i]->priority -
// tuples_priority[prefix_len]; tuples_priority[prefix_len] =
// rules[i]->priority;
// 		}
// 		check_rules_num += ++tuples_key[0][prefix_len][dim_key];

// 		int k = dims_range.reduced[dim][rules[i]->prefix_len[dim]];
// 		dim_check_rules_num[0][k] += tuples_key[0][prefix_len][dim_key];
// 	}
// 	uint64_t cost = check_tuples_num * dim_check_tuple_cost +
// 					check_rules_num * dim_check_rule_cost;
// 	printf("cost %lu : tuple %lu rule %lu\n", cost, check_tuples_num,
// check_rules_num); 	for (int i =0; i < 5; ++i) { 		for (int j = 0;
// j <= dim_prefix_len[i]; ++j) 			printf("%d ",
// dims_range.reduced[i][j]); 		printf("\n");
// 	}
// }

void DimInfo::CalculateDimRangeX(int dim, int x) {
  // if (dim_len_rules_num[dim] == 0) {
  // 	for (int i = x; i <= dim_prefix_len[dim]; ++i)
  // 		tuple_cost_sum[x][i] = 1ULL << 60;
  // 	return;
  // }
  for (int i = 0; i < rules_num; ++i)
    if (rules[i]->prefix_len[dim] >= x)
      for (int j = 0; j < 5; ++j) {
        int k;
        if (j == dim)
          k = dim_prefix_len[j] - x;
        else
          k = dim_prefix_len[j] -
              dims_range.reduced[j][rules[i]->prefix_len[j]];
        rules[i]->reduced_key[j] = (uint64_t)rules[i]->key[j] >> k << k;
      }

  cmp_dim = dim;
  sort(rules.begin(), rules.end(), CmpDimRulePrefixLen);
  tuples_priority.clear();
  tuples_priority_sum = 0;
  memset(dim_check_tuples_num[x], 0, sizeof(dim_check_tuples_num[x]));
  for (int i = 0; i < rules_num; ++i)
    if (rules[i]->prefix_len[dim] >= x) {
      uint32_t prefix_len = GetPrefixLen(rules[i], dims_range, dim);
      if (rules[i]->priority > tuples_priority[prefix_len]) {
        tuples_priority_sum += rules[i]->priority - tuples_priority[prefix_len];
        tuples_priority[prefix_len] = rules[i]->priority;
      }
      dim_check_tuples_num[x][rules[i]->prefix_len[dim]] = tuples_priority_sum;
    }
  for (int i = x + 1; i <= dim_prefix_len[dim]; ++i)
    dim_check_tuples_num[x][i] =
        max(dim_check_tuples_num[x][i], dim_check_tuples_num[x][i - 1]);

  // Rule
  sort(rules.begin(), rules.end(), CmpDimRulePriority);
  for (int i = 0; i <= dim_prefix_len[dim]; ++i) tuples_key[i].clear();

  memset(dim_check_rules_num[x], 0, sizeof(dim_check_rules_num[x]));
  for (int i = 0; i < rules_num; ++i)
    if (rules[i]->prefix_len[dim] >= x) {
      uint32_t prefix_len = GetPrefixLen(rules[i], dims_range, dim);
      DimKey dim_key = GetReducedDimKey(rules[i]);
      for (int k = rules[i]->prefix_len[dim]; k <= dim_prefix_len[dim]; ++k) {
        int num = ++tuples_key[k][prefix_len][dim_key];
        dim_check_rules_num[x][k] += num;
      }
    }

  for (int i = x; i <= dim_prefix_len[dim]; ++i)
    tuple_cost_sum[x][i] = dim_check_tuples_num[x][i] * dim_check_tuple_cost +
                           dim_check_rules_num[x][i] * dim_check_rule_cost;

  // for (int i = x; i <= dim_prefix_len[dim]; ++i)
  // 	printf("%lu ", tuple_cost_sum[x][i]);
  // printf("\n");
}

void DimInfo::GetDpRange(int i, int j, int dim) {
  if (dp_cost[i][j] == tuple_cost_sum[i][j]) {
    for (int k = i; k <= j; ++k) dims_range.reduced[dim][k] = i;
    return;
  }

  for (int k = i; k < j; ++k)
    if (dp_cost[i][j] == dp_cost[i][k] + dp_cost[k + 1][j]) {
      GetDpRange(i, k, dim);
      GetDpRange(k + 1, j, dim);
      break;
    }
}

void DimInfo::DynamicProgrammingX(int dim) {
  int len = dim_prefix_len[dim];
  for (int p = 0; p <= len; ++p)
    for (int i = 0; i + p <= len; ++i) {
      int j = i + p;
      dp_cost[i][j] = tuple_cost_sum[i][j];
      dp_tuple[i][j] = dim_check_tuples_num[i][j];
      dp_rule[i][j] = dim_check_rules_num[i][j];
      for (int k = i; k < j; ++k)
        if (dp_cost[i][j] > dp_cost[i][k] + dp_cost[k + 1][j]) {
          dp_cost[i][j] = dp_cost[i][k] + dp_cost[k + 1][j];
          dp_tuple[i][j] = dp_tuple[i][k] + dp_tuple[k + 1][j];
          dp_rule[i][j] = dp_rule[i][k] + dp_rule[k + 1][j];
        }
    }
  // printf("dp_cost %lu : tuple %u rule %u\n", dp_cost[0][len],
  // dp_tuple[0][len], dp_rule[0][len]); printf("dp_cost %lu\n",
  // dp_cost[0][32]);
}

uint64_t DimInfo::CalculateDimRange(int dim) {
  for (int i = 0; i < rules_num; ++i)
    for (int j = 0; j < 5; ++j) {
      int k = dim_prefix_len[j] - rules[i]->prefix_len[j];
      rules[i]->reduced_key[j] = (uint64_t)rules[i]->key[j] >> k << k;
    }
  memset(dim_len_rules_num, 0, sizeof(dim_len_rules_num));
  for (int i = 0; i < rules_num; ++i)
    ++dim_len_rules_num[rules[i]->prefix_len[dim]];

  memset(tuple_cost_sum, 0, sizeof(tuple_cost_sum));
  for (int i = dim_prefix_len[dim]; i >= 0; --i) CalculateDimRangeX(dim, i);

  DynamicProgrammingX(dim);
  GetDpRange(0, dim_prefix_len[dim], dim);
  return dp_cost[0][dim_prefix_len[dim]];
}

void DimInfo::Free() {
  for (int i = 0; i < rules_num; ++i) free(rules[i]);
  free(this);
}

DimsRange GetDimRange(vector<Rule_DT_MT *> &rules, int prefix_dims_num) {
  // printf("DimRange %d\n", prefix_dims_num);
  DimsRange dims_range;
  if (rules.size() == 0) {
    dims_range.Init();
    return dims_range;
  }
  DimInfo *dim_info = new DimInfo();
  dim_info->GetDimRules(rules);
  dim_info->dims_range.Init();

  uint64_t cost = 0, pre_cost = 1ULL << 60;
  for (int round = 0; round < 100; ++round) {
    std::cout << "\n\n\nround " << round << '\n';

    for (int dim = 0; dim < prefix_dims_num; ++dim) {
      // 重新計算該維度的區間成本
      cost = dim_info->CalculateDimRange(dim);

      // 輸出每個 prefix 長度
      for (int i = 0; i < dim_prefix_len[dim]; ++i) {
        std::cout << dim_info->dims_range.reduced[dim][i] << ' ';
      }
      std::cout << '\n';
    }

    std::cout << "-----pre_cost " << pre_cost << " cost " << cost << '\n';

    // 若成本意外上升，輸出至錯誤流並結束程式
    if (cost > pre_cost) {
      std::cerr << "Wrong cost " << cost << " pre_cost " << pre_cost << '\n';
      std::exit(EXIT_FAILURE);  // EXIT_FAILURE 來自 <cstdlib>
    }

    // 收斂判斷：成本只剩 5% 以內的改善就跳出
    if (cost > pre_cost * 0.95) break;

    pre_cost = cost;
  }

  dims_range = dim_info->dims_range;
  dim_info->Free();
  return dims_range;
}
