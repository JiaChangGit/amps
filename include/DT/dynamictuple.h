#ifndef DYNAMICTUPLE_H
#define DYNAMICTUPLE_H

#include "dynamictuple-dims.h"
#include "dynamictuple-ranges.h"
#include "elementary_DT_MT.h"
#include "tuple.h"

using namespace std;

class DynamicTuple : public Classifier {
 public:
  int Create(vector<Rule_DT_MT *> &rules, bool insert);

  int InsertRule(Rule_DT_MT *rule);
  int DeleteRule(Rule_DT_MT *rule);
  int Lookup(Trace *trace, int priority);
  int LookupAccess(Trace *trace, int priority, Rule_DT_MT *ans_rule,
                   ProgramState *program_state);

  int Reconstruct();
  uint64_t MemorySize();
  int CalculateState(ProgramState *program_state);
  int GetRules(vector<Rule_DT_MT *> &rules);
  int Free(bool free_self);
  int Test(void *ptr);

  // string method_name; // JIA
  vector<TupleRange> tuple_ranges;
  char prefix_down[33][33]
                  [2];  // the first dimension is the prefix length of source IP
                        // the second dimension is the prefix length of
                        // destination IP the third dimension [0] is the reduced
                        // prefix length of source IP the third dimension [1] is
                        // the reduced prefix length of destination IP
  int threshold;        // port_hashtable

  uint32_t prefix_mask[33];
  bool use_port_hash_table;

  Tuple **tuples_arr;
  map<uint32_t, Tuple *> tuples_map;
  int tuples_num;
  int max_tuples_num;
  int rules_num;
  int max_priority;

  double dt_time;

  void InsertTuple(Tuple *tuple);
  void SortTuples();
};

int FreeRules(vector<Rule_DT_MT *> &rules);
static int port_bit_mask[17][2] = {
    {0, 0xffff},     {0x1, 0xfffe},    {0x3, 0xfffc},    {0x7, 0xfff8},
    {0xf, 0xfff0},   {0x1f, 0xffe0},   {0x3f, 0xffc0},   {0x7f, 0xff80},
    {0xff, 0xff00},  {0x1ff, 0xfe00},  {0x3ff, 0xfc00},  {0x7ff, 0xf800},
    {0xfff, 0xf000}, {0x1fff, 0xe000}, {0x3fff, 0xc000}, {0x7fff, 0x8000},
    {0xffff, 0}};
struct PrefixRange {
  uint32_t low;
  uint32_t high;
  uint32_t prefix_len;
};
vector<PrefixRange> GetPortMask(int port_start, int port_end);
vector<Rule_DT_MT *> RulesPortPrefix(vector<Rule_DT_MT *> &rules,
                                     bool free_rules);

#endif
