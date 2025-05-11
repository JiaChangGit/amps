#ifndef TRIE_H
#define TRIE_H

#include <algorithm>
#include <cstdint>
#include <cstdlib>

#include "elementary_DT_MT.h"

#define INSERT 1
#define DELETE 2

using namespace std;

struct TrieNode {
  TrieNode *child[2];
  int priority;
  bool solid_node;

  static TrieNode *Create(int _priority, bool _solid_node) {
    TrieNode *trie_node = (TrieNode *)malloc(sizeof(TrieNode));
    trie_node->child[0] = nullptr;
    trie_node->child[1] = nullptr;
    trie_node->priority = _priority;
    trie_node->solid_node = _solid_node;
    return trie_node;
  }

  int CountNum() {
    if (this == nullptr) return 0;
    return 1 + (child[0] ? child[0]->CountNum() : 0) +
           (child[1] ? child[1]->CountNum() : 0);
  }

  void Free() {
    if (this != nullptr) free(this);
  }

  void FreeAll() {
    if (this == nullptr) return;
    if (child[0]) child[0]->FreeAll();
    if (child[1]) child[1]->FreeAll();
    free(this);
  }
};

class Trie {
 public:
  int Init() {
    root = root->Create(0, true);
    solid_node_num = 1;
    return 0;
  }

  int InsertRule(uint32_t ip, uint8_t prefix_len, int priority) {
    return Update(ip, prefix_len, priority, INSERT);
  }

  int DeleteRule(uint32_t ip, uint8_t prefix_len) {
    return Update(ip, prefix_len, 0, DELETE);
  }

  int Lookup(uint32_t ip, int max_prefix_len) {
    TrieNode *node = root;
    int index = 0;
    for (; index < max_prefix_len; ++index) {
      if (!node) return index;
      int bit = (ip >> (31 - index)) & 1;
      node = node->child[bit];
    }
    return index;
  }

  uint64_t Memory() {
    uint64_t size = sizeof(Trie);
    size += root->CountNum() * sizeof(TrieNode);
    return size;
  }

  int Free() {
    root->FreeAll();
    return 0;
  }

  int Test(void *ptr) { return 0; }

 private:
  int Update(uint32_t ip, uint8_t prefix_len, int priority, int operation) {
    if (prefix_len == 0) {
      return 0;  // Note: Original code had unreachable printf and exit
    }
    TrieNode *pre_node = nullptr;
    TrieNode *node = root;
    pre_nodes[0] = node;
    pre_nodes_num = 1;
    for (int i = 0; i < prefix_len; ++i) {
      if (node->solid_node) pre_node = node;
      int bit = (ip >> (31 - i)) & 1;
      if (node->child[bit] == nullptr)
        node->child[bit] = TrieNode::Create(0, false);
      node = node->child[bit];
      pre_nodes[pre_nodes_num] = node;
      ++pre_nodes_num;
    }

    if (operation == INSERT) {
      if (node->solid_node) {
        node->priority = max(node->priority, priority);
      } else {
        node->priority = priority;
      }
      node->solid_node = true;
    } else if (operation == DELETE) {
      // Original code had empty DELETE operation
    }
    return 0;
  }

  TrieNode *root;
  int solid_node_num;
  TrieNode *pre_nodes[129];
  int pre_nodes_num;
};

#endif
