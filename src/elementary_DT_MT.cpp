#include "elementary_DT_MT.h"

using namespace std;

bool SameRule(Rule_DT_MT *rule1, Rule_DT_MT *rule2) {
  for (int i = 0; i < 5; ++i)
    for (int j = 0; j < 2; ++j)
      if (rule1->range[i][j] != rule2->range[i][j]) return false;
  for (int i = 0; i < 5; ++i)
    if (rule1->prefix_len[i] != rule2->prefix_len[i]) return false;
  if (rule1->priority != rule2->priority) return false;
  return true;
}

bool CmpRulePriority(Rule_DT_MT *rule1, Rule_DT_MT *rule2) {
  return rule1->priority > rule2->priority;
}

uint64_t GetRunTimeUs(timeval timeval_start, timeval timeval_end) {  // us
  return 1000000 * (timeval_end.tv_sec - timeval_start.tv_sec) +
         timeval_end.tv_usec - timeval_start.tv_usec;
}

uint64_t GetAvgTime(vector<uint64_t> &lookup_times) {
  int num = lookup_times.size();
  if (num == 0) return 0;
  sort(lookup_times.begin(), lookup_times.end());
  int l = num / 4;
  int r = num - l;
  // printf("GetAvgTime num %d l %d r %d\n", num, l, r);
  uint64_t sum = 0;
  for (int i = l; i < r; ++i) sum += lookup_times[i];
  return sum / (r - l);
}

void ProgramState::AccessClear() {
  access_tuples.ClearNum();
  access_tables.ClearNum();
  access_nodes.ClearNum();
  access_rules.ClearNum();
}
void ProgramState::AccessCal() {
  max_access_all = max(max_access_all, access_tuples.num + access_tables.num +
                                           access_nodes.num + access_rules.num);
  max_access_tuple_node_rule =
      max(max_access_tuple_node_rule,
          access_tuples.num + access_nodes.num + access_rules.num);
  max_access_node_rule =
      max(max_access_node_rule, access_nodes.num + access_rules.num);

  access_tuples.Update();
  access_tables.Update();
  access_nodes.Update();
  access_rules.Update();
}
