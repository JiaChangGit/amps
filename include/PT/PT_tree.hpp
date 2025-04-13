/*
 *	MIT License
 *
 *	Copyright(c) 2022 ShangHai Jiao Tong Univiersity CIT Laboratory.
 *
 *	Permission is hereby granted, free of charge, to any person obtaining a
 *copy of this softwareand associated documentation files(the "Software"), to
 *deal in the Software without restriction, including without limitation the
 *rights to use, copy, modify, merge, publish, distribute, sublicense, and /or
 *sell copies of the Software, and to permit persons to whom the Software is
 *	furnished to do so, subject to the following conditions :
 *
 *	The above copyright noticeand this permission notice shall be included
 *in all copies or substantial portions of the Software.
 *
 *	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 *OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL
 *THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 *IN THE SOFTWARE.
 */

#ifndef __PT_TREE_
#define __PT_TREE_
#include <unistd.h>

#include <cmath>
#include <cstddef>
#include <list>
#include <random>

#include "PT_data_structure.hpp"
#include "basis.hpp"

#define SIP_1 0
#define SIP_2 1
#define SIP_3 2
#define SIP_4 3
#define DIP_1 4
#define DIP_2 5
#define DIP_3 6
#define DIP_4 7

#define AVG_ACC_INNER_COST 100
#define AVG_ACC_LEAF_COST 2
#define AVG_ACC_RULE_COST 1
#define CACU_SCORE(x) (0.00012 * (x) * (x))

using namespace std;
using namespace PT;
namespace PT {
extern uint8_t maskHash[33][4];
extern uint32_t maskBit[33];

struct IpTable {
  uint32_t pri;
  uint32_t mask;
  vector<short> table;
  vector<pair<uint32_t, void*>> child;  // first: pri second: pointer
  IpTable(uint32_t n) : table(1 << n, -1), pri(0), mask(n) {}
};

struct IpNode {
  uint32_t id;
  uint16_t layer;
  uint8_t field;            // 0-3: sip 1-4; 4-7: dip 1-4
  bool childType;           // 0: innernode; 1: leafnode
  list<IpTable> tableList;  // first: pri second: table
  IpNode(uint8_t _field, bool _cType, uint16_t _layer, uint32_t _id)
      : field(_field), childType(_cType), layer(_layer), id(_id) {}
};

struct IpChild {
  void* pointer;
  int pri;
  IpChild() : pointer(nullptr) {}
};
struct IpNode_static {
  uint32_t id;
  uint16_t layer;
  uint8_t field;   // 0-3: sip 1-4; 4-7: dip 1-4
  bool childType;  // 0: innernode; 1: leafnode
  IpChild child[257];
  IpNode_static(uint8_t _field, bool _cType, uint16_t _layer, uint32_t _id)
      : field(_field), childType(_cType), layer(_layer), id(_id) {}
};
struct LeafNode {
  vector<PT_Rule> rule;
  LeafNode() {}
  LeafNode(vector<PT_Rule>& _rule) : rule(_rule) {}
};
struct PortNode_static {
  uint32_t id;
  short table[32769];
  vector<pair<uint32_t, LeafNode*>> child;
  PortNode_static(uint32_t _id) : id(_id) {
    for (int i = 0; i < 32769; ++i) table[i] = -1;
  }
};
struct ProtoNode {
  vector<short> table;
  vector<pair<uint32_t, void*>> child;
  ProtoNode() : table(256, -1) {}
};

struct ACL_LOG {
  int rules;
  int tables;
  int innerNodes;
  int leafNodes;
  vector<void*> ipNodeList;
  vector<LeafNode*> pLeafNodeList;
  vector<void*> portNodeList;
  vector<LeafNode*> aLeafNodeList;
  ACL_LOG() : rules(0), tables(0), innerNodes(0), leafNodes(0) {}
};

/// ====== ///
int check_correct(PT_Rule& a, PT_Packet& b);
void setmaskHash();

uint64_t reverse_byte(uint64_t x);
/// ====== ///

class CacuInfo {
 public:
  vector<PT_CacuRule*> cRules;
  vector<vector<uint8_t>> fields;
  int best_fields_id;
  double min_cost;

  CacuInfo(vector<PT_Rule>& _rules);
  void reset_cRules();
  void read_fields();

  vector<uint8_t> cacu_best_fields();
  double cacu_cost(vector<uint8_t>& _fields);
  uint32_t cacu_in_node(int _start, int _end);
  double cacu_in_leaf(int _start, int _end);
  double cacu_score(uint32_t x);
};

class PTtree {
 private:
  vector<uint8_t> layerFields;
  int portField, portStep;

 public:
  void* pTree;
  ProtoNode* aTree;
  bool pt_is_modify;
  bool at_is_modify;
  int totalNodes;
  vector<void*> ipNodeList;
  vector<void*> portNodeList;
  vector<LeafNode*> pLeafNodeList;
  vector<LeafNode*> aLeafNodeList;
  vector<LeafNode*> re_leaf;

  PTtree(vector<uint8_t>& list, int _portField);
  ~PTtree();

  void freeStaticNode(IpNode_static* node);
  void freeNode(IpNode* node);

  void insert(PT_Rule& r);
  void insert_up(PT_Rule& r);

  bool remove(PT_Rule& r);

  int search(const PT_Packet& p);
  int search_with_log(PT_Packet& p, ACL_LOG& log);

  bool update(vector<PT_Rule>& rules, int num, Timer& timer);

  void print_node_info(const size_t rules);

  size_t get_ipNode_mem(IpNode* node);
  size_t get_leafNode_mem(LeafNode* node);
  size_t get_static_mem(IpNode_static* node);
  size_t get_mem(IpNode* node);
  size_t mem();
};
}  // namespace PT
#endif  // !_PT_TREE_
