#include "tuple.h"

using namespace std;

Tuple::Tuple(uint32_t prefix_src, uint32_t prefix_dst,
             bool _use_port_hash_table) {
  prefix_len[0] = prefix_src;
  prefix_len[1] = prefix_dst;
  prefix_len_zero[0] = 32 - prefix_src;
  prefix_len_zero[1] = 32 - prefix_dst;
  for (int i = 0; i < 2; ++i)
    prefix_mask[i] = ~((1ULL << (32 - prefix_len[i])) - 1);
  max_priority = 0;
  rules_num = 0;
  use_port_hash_table = _use_port_hash_table;
  hash_table.Init(32, false, use_port_hash_table);
}

int Tuple::InsertRule(Rule_DT_MT *rule) {
  // uint32_t src_ip = rule.range[0][0] & prefix_mask[0];
  // uint32_t dst_ip = rule.range[1][0] & prefix_mask[1];
  uint32_t src_ip = ((uint64_t)rule->range[0][0]) >> prefix_len_zero[0];
  uint32_t dst_ip = ((uint64_t)rule->range[1][0]) >> prefix_len_zero[1];
  uint64_t key = ((uint64_t)src_ip << 32) | dst_ip;
  uint32_t hash = Hash32_2(src_ip, dst_ip);

  if (hash_table.InsertRule(rule, key, hash) > 0) return 1;
  ++rules_num;
  max_priority = hash_table.max_priority;
  return 0;
}

int Tuple::DeleteRule(Rule_DT_MT *rule) {
  // uint32_t src_ip = rule.range[0][0] & prefix_mask[0];
  // uint32_t dst_ip = rule.range[1][0] & prefix_mask[1];
  uint32_t src_ip = ((uint64_t)rule->range[0][0]) >> prefix_len_zero[0];
  uint32_t dst_ip = ((uint64_t)rule->range[1][0]) >> prefix_len_zero[1];
  uint64_t key = ((uint64_t)src_ip << 32) | dst_ip;
  uint32_t hash = Hash32_2(src_ip, dst_ip);

  if (hash_table.DeleteRule(rule, key, hash) > 0) return 1;
  --rules_num;
  max_priority = hash_table.max_priority;
  return 0;
}

uint64_t Tuple::MemorySize() {
  uint64_t memory_size = sizeof(Tuple);
  memory_size += hash_table.MemorySize() - sizeof(HashTable);
  return memory_size;
}

int Tuple::GetRules(vector<Rule_DT_MT *> &rules) {
  hash_table.GetRules(rules);
  return 0;
}

int Tuple::Free(bool free_self) {
  hash_table.Free(false);
  if (free_self) free(this);
  return 0;
}

int Tuple::CalculateState(ProgramState *program_state) {
  // printf("%d %d : rules_num = %d max_priority = %d\n", prefix_len[0],
  // prefix_len[1], rules_num, max_priority);
  hash_table.CalculateState(program_state);
  return 0;
}

int Tuple::Test(void *ptr) { return 0; }
