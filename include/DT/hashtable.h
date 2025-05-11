#ifndef HASHTABLE_H
#define HASHTABLE_H

#include "elementary_DT_MT.h"
#include "hashnode.h"

#define HASHTABLEMAX 0.85
#define HASHTABLEMIN 0.2

using namespace std;

struct HashNode;

struct HashTable {
  uint32_t hash_node_num;
  uint32_t max_hash_node_num;
  uint32_t min_hash_node_num;
  uint32_t mask;  // sizeof HashNode - 1
  HashNode **hash_node_arr;
  int max_priority;
  bool is_port_hash_table;
  bool use_port_hash_table;

  int Init(int size, bool _is_port_hash_table, bool _use_port_hash_table);
  int InsertHashNode(HashNode *hash_node);
  HashNode *PickHashNode(uint64_t key, uint32_t hash);
  int HashTableResize(uint32_t size);

  int InsertRule(Rule_DT_MT *rule, uint64_t key, uint32_t hash);
  int DeleteRule(Rule_DT_MT *rule, uint64_t key, uint32_t hash);
  uint64_t MemorySize();
  int CalculateState(ProgramState *program_state);
  int GetRules(vector<Rule_DT_MT *> &rules);
  int Free(bool free_self);
  int Test(void *ptr);
};

#endif
