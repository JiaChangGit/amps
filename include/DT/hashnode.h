#ifndef HASHNODE_H
#define HASHNODE_H

#include "elementary_DT_MT.h"
#include "hash.h"
#include "hashtable.h"

using namespace std;

struct HashTable;

struct RuleNode {
  Rule_DT_MT *rule;
  uint32_t src_ip_begin, src_ip_end;
  uint32_t dst_ip_begin, dst_ip_end;
  uint16_t src_port_begin, src_port_end;
  uint16_t dst_port_begin, dst_port_end;
  uint8_t protocol_begin, protocol_end;
  int priority;
  RuleNode *next;
  RuleNode(Rule_DT_MT *_rule) {
    rule = _rule;
    src_ip_begin = rule->range[0][0];
    src_ip_end = rule->range[0][1];
    dst_ip_begin = rule->range[1][0];
    dst_ip_end = rule->range[1][1];
    src_port_begin = rule->range[2][0];
    src_port_end = rule->range[2][1];
    dst_port_begin = rule->range[3][0];
    dst_port_end = rule->range[3][1];
    protocol_begin = rule->range[4][0];
    protocol_end = rule->range[4][1];
    priority = rule->priority;
    next = nullptr;
  }
  uint64_t MemorySize() { return sizeof(RuleNode); }

  int Free(bool free_self) {
    if (free_self) free(this);
    return 0;
  }
};

// struct PortHashTable;

struct HashNode {
  RuleNode *rule_node;
  HashNode *next;

  uint64_t key;
  uint32_t hash;
  uint32_t rules_num;
  int max_priority;

  bool begin_port_hash_table;
  bool has_port_hash_table;
  HashTable *port_hash_table[2];

  HashNode(uint64_t _key, uint32_t _hash);
  void UpdatePriority();
  int InsertRule(Rule_DT_MT *rule, bool is_port_hash_table,
                 bool use_port_hash_table);
  int DeleteRule(Rule_DT_MT *rule, bool is_port_hash_table,
                 bool use_port_hash_table);
  uint64_t MemorySize();
  int CalculateState(ProgramState *program_state);
  int GetRules(vector<Rule_DT_MT *> &rules);
  int Free(bool free_self);
  int Test(void *ptr);
};

#endif
