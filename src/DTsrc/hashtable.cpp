#include "hashtable.h"

using namespace std;

int HashTable::Init(int size, bool _is_port_hash_table,
                    bool _use_port_hash_table) {
  hash_node_num = 0;
  max_hash_node_num = size * HASHTABLEMAX;
  min_hash_node_num = size * HASHTABLEMIN;
  mask = size - 1;
  hash_node_arr = (HashNode **)malloc(sizeof(HashNode *) * (mask + 1));
  for (int i = 0; i < size; ++i) hash_node_arr[i] = nullptr;
  max_priority = 0;
  is_port_hash_table = _is_port_hash_table;
  use_port_hash_table = _use_port_hash_table;
  return 0;
}

int HashTable::InsertHashNode(HashNode *hash_node) {
  if (hash_node_num == max_hash_node_num) HashTableResize((mask + 1) << 1);
  ++hash_node_num;
  uint32_t index = hash_node->hash & mask;
  HashNode *pre = hash_node_arr[index];

  if (!pre || hash_node->max_priority > pre->max_priority) {
    hash_node_arr[index] = hash_node;
    hash_node->next = pre;
    return 0;
  }
  HashNode *next = pre->next;
  while (true) {
    if (!next || hash_node->max_priority > next->max_priority) {
      pre->next = hash_node;
      hash_node->next = next;
      return 0;
    }
    pre = next;
    next = pre->next;
  }
  return 1;
}

HashNode *HashTable::PickHashNode(uint64_t key, uint32_t hash) {
  uint32_t index = hash & mask;
  HashNode *hash_node = hash_node_arr[index];
  HashNode *pre_hash_node = nullptr;
  while (hash_node) {
    if (hash_node->key == key) {
      if (pre_hash_node == nullptr)
        hash_node_arr[index] = hash_node->next;
      else
        pre_hash_node->next = hash_node->next;
      --hash_node_num;
      hash_node->next = nullptr;
      return hash_node;
    }
    pre_hash_node = hash_node;
    hash_node = hash_node->next;
  }
  return nullptr;
}

int HashTable::HashTableResize(uint32_t size) {
  uint32_t origin_size = mask + 1;
  HashNode **origin_hash_node_arr = hash_node_arr;

  hash_node_num = 0;
  max_hash_node_num = size * HASHTABLEMAX;
  min_hash_node_num = size * HASHTABLEMIN;
  mask = size - 1;
  hash_node_arr = (HashNode **)malloc(sizeof(HashNode *) * (mask + 1));
  for (int i = 0; i < size; ++i) hash_node_arr[i] = nullptr;

  HashNode *hash_node, *next;
  for (int i = 0; i < origin_size; ++i) {
    hash_node = origin_hash_node_arr[i];
    while (hash_node) {
      next = hash_node->next;
      hash_node->next = nullptr;
      InsertHashNode(hash_node);
      hash_node = next;
    }
  }
  free(origin_hash_node_arr);
  return 0;
}

int HashTable::InsertRule(Rule_DT_MT *rule, uint64_t key, uint32_t hash) {
  HashNode *hash_node = PickHashNode(key, hash);
  if (!hash_node) hash_node = new HashNode(key, hash);
  hash_node->InsertRule(rule, is_port_hash_table, use_port_hash_table);
  InsertHashNode(hash_node);
  if (rule->priority > max_priority) max_priority = rule->priority;
  return 0;
}

int HashTable::DeleteRule(Rule_DT_MT *rule, uint64_t key, uint32_t hash) {
  HashNode *hash_node = PickHashNode(key, hash);
  if (!hash_node) {
    std::cout << ("Wrong: No such hash_node\n");
    return 1;
  }
  if (hash_node->DeleteRule(rule, is_port_hash_table, use_port_hash_table) >
      0) {
    std::cout << ("Wrong: HashNode DeleteRule\n");
    return 1;
  }
  if (hash_node->rules_num == 0)
    hash_node->Free(true);
  else
    InsertHashNode(hash_node);
  if (rule->priority == max_priority) {
    max_priority = 0;
    for (int i = 0; i <= mask; ++i)
      if (hash_node_arr[i])
        max_priority = max(max_priority, hash_node_arr[i]->max_priority);
  }
  return 0;
}

uint64_t HashTable::MemorySize() {
  uint64_t memory_size = sizeof(HashTable);
  memory_size += sizeof(HashNode *) * (mask + 1);
  HashNode *hash_node = nullptr;
  for (int i = 0; i <= mask; ++i) {
    hash_node = hash_node_arr[i];
    while (hash_node) {
      memory_size += hash_node->MemorySize();
      hash_node = hash_node->next;
    }
  }
  return memory_size;
}

int HashTable::GetRules(vector<Rule_DT_MT *> &rules) {
  HashNode *hash_node = nullptr, *next_hash_node = nullptr;
  for (int i = 0; i <= mask; ++i) {
    hash_node = hash_node_arr[i];
    while (hash_node) {
      next_hash_node = hash_node->next;
      hash_node->GetRules(rules);
      hash_node = next_hash_node;
    }
  }
  return 0;
}

int HashTable::Free(bool free_self) {
  HashNode *hash_node = nullptr, *next_hash_node = nullptr;
  for (int i = 0; i <= mask; ++i) {
    hash_node = hash_node_arr[i];
    while (hash_node) {
      next_hash_node = hash_node->next;
      hash_node->Free(true);
      hash_node = next_hash_node;
    }
  }
  free(hash_node_arr);
  if (free_self) free(this);
  return 0;
}

int HashTable::CalculateState(ProgramState *program_state) {
  program_state->hash_node_num += hash_node_num;
  program_state->bucket_sum += mask + 1;
  HashNode *hash_node = nullptr;
  for (int i = 0; i <= mask; ++i) {
    hash_node = hash_node_arr[i];
    if (hash_node != nullptr) ++program_state->bucket_use;
    while (hash_node) {
      hash_node->CalculateState(program_state);
      hash_node = hash_node->next;
    }
  }
  // printf("hash_node_num = %d\n", hash_node_num);
  return 0;
}

int HashTable::Test(void *ptr) { return 0; }
