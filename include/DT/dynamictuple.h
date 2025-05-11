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

#endif
