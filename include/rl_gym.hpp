#ifndef _RL_GYM_HPP_
#define _RL_GYM_HPP_

#include <string>
#include <vector>

#include "DBT_core.hpp"
#include "KSet_data_structure.hpp"
#include "PT_tree.hpp"
#include "dynamictuple.h"
#include "multilayertuple.h"

class PT_Object {
 private:
  std::vector<uint8_t> set_field;
  int set_port;
  int sampleNum;
  std::vector<PT_Rule> rules;
  std::vector<PT_Packet> packets;
  std::vector<int> slow_Packets;

 public:
  double slow_time;
  // PT_Object() {
  //   build_pt();
  // }
  PT_Object(std::vector<uint8_t> fields, int port, std::vector<PT_Rule>& r,
            const std::vector<PT_Packet>& p)
      : set_field(fields), set_port(port), rules(r), packets(p) {
    build_pt();
  }
  ~PT_Object() {
    rules.clear();
    packets.clear();
    slow_Packets.clear();
    // 確保 PTtree 的記憶體已釋放（假設 PTtree 有內部動態記憶體）
  }
  void build_pt();  // 新增方法
  int get_set_port() const { return set_port; }
  std::vector<uint8_t> get_set_field() const { return set_field; }
  std::vector<int> get_slow_Packets() const { return slow_Packets; }
};

class DBT_Object {
 private:
  int binth;
  double end_bound;
  int top_k;
  int c_bound;
  int sampleNum;
  std::vector<DBT::Rule> rules;
  std::vector<DBT::Packet> packets;
  std::vector<int> slow_Packets;

 public:
  // DBT_Object() : top_k(4), end_bound(0.8), c_bound(32), binth(4) {
  //   build_dbt();
  // }
  DBT_Object(int b, double eb, int tk, int cb, std::vector<DBT::Rule>& r,
             const std::vector<DBT::Packet>& p, const double slow_time)
      : binth(b), end_bound(eb), top_k(tk), c_bound(cb), rules(r), packets(p) {
    build_dbt(slow_time);  // 在構造時自動構建
  }
  ~DBT_Object() {
    rules.clear();
    packets.clear();
    slow_Packets.clear();
  }
  void build_dbt(const double slow_time);  // 新增方法
  int get_binth() const { return binth; }
  double get_end_bound() const { return end_bound; }
  int get_top_k() const { return top_k; }
  int get_c_bound() const { return c_bound; }
  void set_binth(int b) { binth = b; }
  void set_end_bound(double eb) { end_bound = eb; }
  void set_top_k(int tk) { top_k = tk; }
  void set_c_bound(int cb) { c_bound = cb; }
  std::vector<int> get_slow_Packets() const { return slow_Packets; }
};

class DT_Object {
 private:
  int threshold;  // triger port_hashtable ( .use_port_hash_table=true)
  bool is_prefix_5d;
  int sampleNum;
  std::vector<Rule_DT_MT*> rules;
  std::vector<Trace*> packets;
  std::vector<int> slow_Packets;

 public:
  // DT_Object() : threshold(7) {
  //   build_dt();
  // }
  DT_Object(int th, bool is_5d, std::vector<Rule_DT_MT*>& r,
            const std::vector<Trace*>& p, const double slow_time)
      : threshold(th), is_prefix_5d(is_5d), rules(r), packets(p) {
    build_dt(slow_time);
  }
  ~DT_Object();
  void build_dt(const double slow_time);
  int get_threshold() const { return threshold; }
  int get_is_prefix_5d() const { return is_prefix_5d; }
  void set_th(int th) { threshold = th; }
  void set_is_prefix_5d(bool is_5d) { is_prefix_5d = is_5d; }
  std::vector<int> get_slow_Packets() const { return slow_Packets; }
};

class RLGym {
 public:
  void load_KSet_rule_packets(const char* rule_filename,
                              const char* packet_filename);
  PT_Object create_pt_object(std::vector<uint8_t> set_field, int set_port);
  DBT_Object create_dbt_object(int binth, double end_bound, int top_k,
                               int c_bound, const PT_Object& pt_obj);
  DT_Object create_dt_object(int threshold, bool is_prefix_5d,
                             const PT_Object& pt_obj,
                             const DBT_Object& dbt_obj);
  double evaluate_pt(const PT_Object& pt_obj);
  double evaluate_dbt(const PT_Object& pt_obj, const DBT_Object& dbt_obj);
  double evaluate_dt(const PT_Object& pt_obj, const DBT_Object& dbt_obj,
                     const DT_Object& dt_obj);
  PT_Object create_pt_first(std::vector<uint8_t> tmp_in_field, int tmp_port);

 private:
  vector<Rule_KSet> KSet_rule;
  vector<Packet> KSet_packets;
};

#endif
