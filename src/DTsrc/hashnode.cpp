#include "hashnode.h"

using namespace std;

HashNode::HashNode(uint64_t _key, uint32_t _hash) {
  key = _key;
  hash = _hash;
  rules_num = 0;
  max_priority = 0;
  rule_node = nullptr;
  next = nullptr;

  begin_port_hash_table = false;
  has_port_hash_table = false;
  port_hash_table[0] = nullptr;
  port_hash_table[1] = nullptr;
}

void HashNode::UpdatePriority() {
  max_priority = 0;
  if (port_hash_table[0])
    max_priority = max(max_priority, port_hash_table[0]->max_priority);
  if (port_hash_table[1])
    max_priority = max(max_priority, port_hash_table[1]->max_priority);
  if (rule_node) max_priority = max(max_priority, rule_node->rule->priority);
}

int HashNode::InsertRule(Rule_DT_MT *rule, bool is_port_hash_table,
                         bool use_port_hash_table) {
  ++rules_num;
  // insert port_hash_table;
  if (begin_port_hash_table)
    for (int i = 2; i <= 3; ++i)
      if (rule->range[i][0] == rule->range[i][1]) {
        uint64_t key = rule->range[i][0];
        uint32_t hash = Hash16(key);
        if (port_hash_table[i - 2] == nullptr) {
          port_hash_table[i - 2] = new HashTable();
          port_hash_table[i - 2]->Init(32, true, use_port_hash_table);
          has_port_hash_table = true;
        }
        port_hash_table[i - 2]->InsertRule(rule, key, hash);
        UpdatePriority();
        return 0;
      }

  // insert rule_node list
  RuleNode *insert_rule_node = new RuleNode(rule);
  RuleNode *pre_rule_node = nullptr;
  RuleNode *next_rule_node = rule_node;
  while (true) {
    if (!next_rule_node || rule->priority > next_rule_node->rule->priority) {
      if (!pre_rule_node) {
        insert_rule_node->next = rule_node;
        rule_node = insert_rule_node;
        UpdatePriority();
      } else {
        pre_rule_node->next = insert_rule_node;
        insert_rule_node->next = next_rule_node;
      }
      break;
    }
    pre_rule_node = next_rule_node;
    next_rule_node = next_rule_node->next;
  }

  // begin to use port_hash_table
  // if (rules_num >= 7)
  //    printf("rules_num %d, %d %d %d\n", rules_num, begin_port_hash_table,
  //    is_port_hash_table, use_port_hash_table);
  if (rules_num >= 7 && !begin_port_hash_table && !is_port_hash_table &&
      use_port_hash_table) {
    //  std::cout <<("begin to use port_hash_table\n");
    begin_port_hash_table = true;
    RuleNode *origin_rule_node = rule_node;
    RuleNode *next_rule_node = nullptr;
    rule_node = nullptr;
    rules_num = 0;
    max_priority = 0;
    //  std::cout <<("port_hash_table\n");
    while (origin_rule_node) {
      InsertRule(origin_rule_node->rule, is_port_hash_table,
                 use_port_hash_table);
      next_rule_node = origin_rule_node->next;
      origin_rule_node->Free(true);
      origin_rule_node = next_rule_node;
    }
    //  std::cout <<("begin to use port_hash_table end\n");
  }
  return 0;
}

int HashNode::DeleteRule(Rule_DT_MT *rule, bool is_port_hash_table,
                         bool use_port_hash_table) {
  //  std::cout <<("DeleteRule\n");
  // delete in port_hash_table;
  bool delete_success = false;
  if (begin_port_hash_table)
    for (int i = 2; i <= 3; ++i)
      if (rule->range[i][0] == rule->range[i][1]) {
        uint64_t key = rule->range[i][0];
        uint32_t hash = Hash16(key);
        if (port_hash_table[i - 2] == nullptr) {
          std::cout << ("Wrong : no port_hash_table\n");
          return 1;
        }
        if (port_hash_table[i - 2]->DeleteRule(rule, key, hash) > 0) {
          std::cout << ("Wrong : cann't delete in port_hash_table\n");
          exit(1);
          return 1;
        }
        UpdatePriority();
        delete_success = true;
        //  std::cout <<("delete success\n");
        --rules_num;
        break;
      }

  if (!delete_success) {
    // delete in rule_node list
    RuleNode *pre_rule_node = nullptr;
    RuleNode *next_rule_node = rule_node;
    while (next_rule_node) {
      if (SameRule(next_rule_node->rule, rule)) {
        --rules_num;
        if (!pre_rule_node) {
          rule_node = next_rule_node->next;
          UpdatePriority();
        } else {
          pre_rule_node->next = next_rule_node->next;
        }
        next_rule_node->Free(true);
        delete_success = true;
        break;
      }
      pre_rule_node = next_rule_node;
      next_rule_node = next_rule_node->next;
    }
  }
  // printf("rules_num %d delete_success %d\n", rules_num, delete_success);
  if (!is_port_hash_table && begin_port_hash_table && rules_num <= 3) {
    //  std::cout <<("delete port_hash_table\n");
    vector<Rule_DT_MT *> port_hash_table_rules;
    for (int i = 0; i < 2; ++i)
      if (port_hash_table[i]) {
        port_hash_table[i]->GetRules(port_hash_table_rules);
        port_hash_table[i]->Free(true);
        port_hash_table[i] = nullptr;
      }
    begin_port_hash_table = 0;
    has_port_hash_table = 0;
    int port_hash_table_rules_num = port_hash_table_rules.size();
    rules_num -= port_hash_table_rules_num;
    for (int i = 0; i < port_hash_table_rules_num; ++i)
      InsertRule(port_hash_table_rules[i], is_port_hash_table,
                 use_port_hash_table);
    //  std::cout <<("delete port_hash_table end\n");
  }
  return !delete_success;
}

uint64_t HashNode::MemorySize() {
  uint64_t memory_size = sizeof(HashNode);
  RuleNode *rule_node2 = rule_node;
  while (rule_node2) {
    memory_size += rule_node2->MemorySize();
    rule_node2 = rule_node2->next;
  }
  for (int i = 0; i < 2; ++i)
    if (port_hash_table[i]) memory_size += port_hash_table[i]->MemorySize();
  return memory_size;
}

int HashNode::CalculateState(ProgramState *program_state) {
  if (has_port_hash_table) ++program_state->next_layer_num;
  return 0;
}

int HashNode::GetRules(vector<Rule_DT_MT *> &rules) {
  RuleNode *current_rule_node = rule_node, *next_rule_node = nullptr;
  while (current_rule_node) {
    next_rule_node = current_rule_node->next;
    rules.push_back(current_rule_node->rule);
    current_rule_node = next_rule_node;
  }

  for (int i = 0; i < 2; ++i)
    if (port_hash_table[i]) port_hash_table[i]->GetRules(rules);
  return 0;
}

int HashNode::Free(bool free_self) {
  RuleNode *current_rule_node = rule_node, *next_rule_node = nullptr;
  while (current_rule_node) {
    next_rule_node = current_rule_node->next;
    current_rule_node->Free(true);
    current_rule_node = next_rule_node;
  }

  for (int i = 0; i < 2; ++i)
    if (port_hash_table[i]) port_hash_table[i]->Free(true);
  if (free_self) free(this);
  return 0;
}

int HashNode::Test(void *ptr) { return 0; }
