#ifndef TUPLE_H
#define TUPLE_H

#include "elementary_DT_MT.h"
#include "hash.h"
#include "hashtable.h"

using namespace std;

class Tuple {
 public:
  int max_priority;
  int rules_num;
  uint32_t prefix_len[2];
  uint32_t prefix_len_zero[2];
  uint32_t prefix_mask[2];

  HashTable hash_table;
  bool use_port_hash_table;

  Tuple(uint32_t prefix_src, uint32_t prefix_dst, bool _use_port_hash_table);
  int InsertRule(Rule_DT_MT *rule);
  int DeleteRule(Rule_DT_MT *rule);
  uint64_t MemorySize();
  int CalculateState(ProgramState *program_state);
  int GetRules(vector<Rule_DT_MT *> &rules);
  int Free(bool free_self);
  int Test(void *ptr);
};

#endif
