#ifndef MULTILAYERTUPLE_H
#define MULTILAYERTUPLE_H

#include "dynamictuple-dims.h"
#include "elementary_DT_MT.h"
#include "mtuple.h"

#define MULTILATERTUPLE_TYPE 1
#define DYNAMICTUPLEDIMS_TYPE 2

using namespace std;

struct MTuple;

class MultilayerTuple : public Classifier {
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

  int Init(uint32_t _tuple_layer, bool _start_tuple_layer);
  void InsertTuple(MTuple *tuple);
  void SortTuples();
  uint32_t GetReducedPrefix(uint32_t *prefix_len, Rule_DT_MT *rule);

  bool start_tuple_layer;
  uint32_t tuple_layer;

  MTuple **tuples_arr;
  map<uint32_t, MTuple *> tuples_map;
  int tuples_num;
  int max_tuples_num;
  int rules_num;
  int max_priority;
};

#endif
