#include "rl_gym.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>

#include "LinearRegressionModel.hpp"
#include "basis.hpp"
#include "input.hpp"

#define EIGEN_NO_DEBUG  // 關閉 Eigen assert
namespace py = pybind11;
using namespace std;

///////// DBT /////////
#include "DBT_core.hpp"
int DBT::TOP_K = 4;
double DBT::END_BOUND = 0.8;
int DBT::C_BOUND = 32;
int DBT::BINTH = 4;
uint32_t DBT::maskBit[33] = {
    0,          0x80000000, 0xC0000000, 0xE0000000, 0xF0000000, 0xF8000000,
    0xFC000000, 0xFE000000, 0xFF000000, 0xFF800000, 0xFFC00000, 0xFFE00000,
    0xFFF00000, 0xFFF80000, 0xFFFC0000, 0xFFFE0000, 0xFFFF0000, 0xFFFF8000,
    0xFFFFC000, 0xFFFFE000, 0xFFFFF000, 0xFFFFF800, 0xFFFFFC00, 0xFFFFFE00,
    0xFFFFFF00, 0xFFFFFF80, 0xFFFFFFC0, 0xFFFFFFE0, 0xFFFFFFF0, 0xFFFFFFF8,
    0xFFFFFFFC, 0xFFFFFFFE, 0xFFFFFFFF};

uint32_t DBT::getBit[32] = {
    0x80000000, 0x40000000, 0x20000000, 0x10000000, 0x08000000, 0x04000000,
    0x02000000, 0x01000000, 0x00800000, 0x00400000, 0x00200000, 0x00100000,
    0x00080000, 0x00040000, 0x00020000, 0x00010000, 0x00008000, 0x00004000,
    0x00002000, 0x00001000, 0x00000800, 0x00000400, 0x00000200, 0x00000100,
    0x00000080, 0x00000040, 0x00000020, 0x00000010, 0x00000008, 0x00000004,
    0x00000002, 0x00000001};
///////// DBT /////////

// DT_Object 解構函數
DT_Object::~DT_Object() {
  rules.clear();
  packets.clear();
  slow_Packets.clear();
  for (auto rule : rules) {
    free(rule);
  }
  for (auto packet : packets) {
    free(packet);
  }
}

/////////////////
// ===============================
// Convert Packet -> PT_Packet
// ===============================
vector<PT_Packet> convertToPTPackets(const vector<Packet>& packets) {
  vector<PT_Packet> pt_packets;

  for (const auto& pkt : packets) {
    PT_Packet pt{};
    uint32_t sip = pkt[0];
    uint32_t dip = pkt[1];

    // IP 順序修正：big endian
    // bytes_reverse(sip, pt.source_ip);
    // bytes_reverse(dip, pt.destination_ip);
    bytes_allocate(sip, pt.source_ip);
    bytes_allocate(dip, pt.destination_ip);
    pt.source_port = static_cast<uint16_t>(pkt[2]);
    pt.destination_port = static_cast<uint16_t>(pkt[3]);
    pt.protocol = pkt[4];

    pt_packets.emplace_back(pt);
  }
  pt_packets.shrink_to_fit();
  return pt_packets;
}

// ===============================
// Convert Rule_KSet -> PT_Rule
// ===============================
vector<PT_Rule> convertToPTRules(const vector<Rule_KSet>& rules) {
  vector<PT_Rule> pt_rules;

  for (const auto& r : rules) {
    PT_Rule pt{};
    pt.pri = r.pri;

    // protocol
    pt.protocol[0] = (r.range[4][0] == r.range[4][1]) ? 0xFF : 0x00;
    pt.protocol[1] = static_cast<unsigned char>(r.range[4][0]);

    // source IP
    pt.source_mask = static_cast<unsigned char>(r.prefix_length[0]);
    // bytes_reverse(r.range[0][0], pt.source_ip);
    bytes_allocate(r.range[0][0], pt.source_ip);
    // destination IP
    pt.destination_mask = static_cast<unsigned char>(r.prefix_length[1]);
    // bytes_reverse(r.range[1][0], pt.destination_ip);
    bytes_allocate(r.range[1][0], pt.destination_ip);
    // ports
    pt.source_port[0] = static_cast<uint16_t>(r.range[2][0]);
    pt.source_port[1] = static_cast<uint16_t>(r.range[2][1]);
    pt.destination_port[0] = static_cast<uint16_t>(r.range[3][0]);
    pt.destination_port[1] = static_cast<uint16_t>(r.range[3][1]);

    pt_rules.emplace_back(pt);
  }
  pt_rules.shrink_to_fit();
  return pt_rules;
}

vector<DBT::Packet> convertToDBTPackets(const vector<Packet>& packets) {
  vector<DBT::Packet> dbt_packets;

  for (const Packet& p : packets) {
    DBT::Packet dbt_p;
    // IP 順序修正：big endian
    uint32_t sip = p[0];
    uint32_t dip = p[1];
    // bytes_reverse(sip, dbt_p.ip.i_8.sip);
    // bytes_reverse(dip, dbt_p.ip.i_8.dip);
    dbt_p.ip.i_32.sip = p[0];
    dbt_p.ip.i_32.dip = p[1];
    dbt_p.Port[0] = static_cast<uint16_t>(p[2]);
    dbt_p.Port[1] = static_cast<uint16_t>(p[3]);
    dbt_p.protocol = p[4];

    dbt_packets.emplace_back(dbt_p);
  }
  dbt_packets.shrink_to_fit();
  return dbt_packets;
}

vector<DBT::Rule> convertToDBTRules(const vector<Rule_KSet>& rules) {
  vector<DBT::Rule> dbt_rules;

  for (const Rule_KSet& r : rules) {
    DBT::Rule dbt_r;
    dbt_r.pri = r.pri;

    // Protocol
    if (r.range[4][0] == r.range[4][1]) {
      dbt_r.protocol.val = static_cast<uint8_t>(r.range[4][0]);
      dbt_r.protocol.mask = 0xFF;
    } else {
      dbt_r.protocol.val = 0;
      dbt_r.protocol.mask = 0;
    }

    dbt_r.sip_length = static_cast<uint8_t>(r.prefix_length[0]);
    dbt_r.dip_length = static_cast<uint8_t>(r.prefix_length[1]);

    dbt_r.Port[0][0] = static_cast<uint16_t>(r.range[2][0]);
    dbt_r.Port[0][1] = static_cast<uint16_t>(r.range[2][1]);
    dbt_r.Port[1][0] = static_cast<uint16_t>(r.range[3][0]);
    dbt_r.Port[1][1] = static_cast<uint16_t>(r.range[3][1]);

    // 明確拆解成 bytes（避免 Endian 問題）
    // bytes_reverse(r.range[0][0], dbt_r.ip.i_8.sip);
    // bytes_reverse(r.range[1][0], dbt_r.ip.i_8.dip);
    dbt_r.ip.i_32.sip = r.range[0][0];
    dbt_r.ip.i_32.dip = r.range[1][0];

    dbt_r.mask.i_32.smask = DBT::maskBit[dbt_r.sip_length];
    dbt_r.mask.i_32.dmask = DBT::maskBit[dbt_r.dip_length];

    dbt_rules.emplace_back(dbt_r);
  }
  dbt_rules.shrink_to_fit();
  return dbt_rules;
}

vector<Trace*> convertPackets_KSet2DTMT(const vector<Packet>& packets) {
  vector<Trace*> traces;
  for (const auto& packet : packets) {
    Trace* trace = (Trace*)malloc(sizeof(Trace));
    // Copy key data for up to 5 dimensions
    for (size_t i = 0; i < 5; ++i) {
      trace->key[i] = packet[i];
    }
    traces.emplace_back(trace);
  }
  traces.shrink_to_fit();
  return traces;
}

vector<Rule_DT_MT*> convertRules_KSetToDTMT(
    const vector<Rule_KSet>& ksetRules) {
  vector<Rule_DT_MT*> dtmtRules;
  for (const auto& ksetRule : ksetRules) {
    Rule_DT_MT* dtmtRule = (Rule_DT_MT*)malloc(sizeof(Rule_DT_MT));
    memset(dtmtRule, 0, sizeof(Rule_DT_MT));

    // Copy range data for up to 5 dimensions
    for (size_t i = 0; i < 5 && i < ksetRule.dim; ++i) {
      dtmtRule->range[i][0] = ksetRule.range[i][LowDim];
      dtmtRule->range[i][1] = ksetRule.range[i][HighDim];
      dtmtRule->prefix_len[i] = ksetRule.prefix_length[i];
    }

    // Copy priority
    dtmtRule->priority = ksetRule.priority;

    dtmtRules.emplace_back(dtmtRule);
  }
  dtmtRules.shrink_to_fit();
  return dtmtRules;
}

vector<Rule_KSet> convertRules_DTMTtoKSet(
    const vector<Rule_DT_MT*>& dtmtRules) {
  vector<Rule_KSet> ksetRules;
  for (const auto& dtmtRule : dtmtRules) {
    Rule_KSet ksetRule;
    // Set dimension (assuming max 5 as per original conversion)
    ksetRule.dim = 5;

    // Copy range data for up to 5 dimensions
    for (size_t i = 0; i < 5; ++i) {
      ksetRule.range[i][LowDim] = dtmtRule->range[i][0];
      ksetRule.range[i][HighDim] = dtmtRule->range[i][1];
      ksetRule.prefix_length[i] = dtmtRule->prefix_len[i];
    }

    // Copy priority
    ksetRule.priority = dtmtRule->priority;

    ksetRules.emplace_back(ksetRule);
  }
  ksetRules.shrink_to_fit();
  return ksetRules;
}

vector<int> getIntersection(const vector<int>& V1, const vector<int>& V2) {
  if (0 >= V1.size()) cerr << "\n0>=V1 WRONG!!\n";
  if (0 >= V2.size()) cerr << "\n0>=V2 WRONG!!\n";
  vector<int> result;
  size_t i = 0, j = 0;
  while (i < V1.size() && j < V2.size()) {
    if (V1[i] < V2[j]) {
      ++i;
    } else if (V1[i] > V2[j]) {
      ++j;
    } else {  // V1[i] == V2[j]
      result.push_back(V1[i]);
      ++i;
      ++j;
    }
  }
  return result;
}

void PT_Object::build_pt() {
  sampleNum = packets.size();
  if (0 >= sampleNum) std::cerr << "\nPT build WRONG(0 >= sampleNum)!!\n";
  PTtree tree(set_field, set_port);  // 建 PTtree
  for (auto&& r : rules) {
    tree.insert(r);  // 插入規則
  }
  Eigen::VectorXd PT_y(sampleNum);
  Timer timer;
#if TIMER_METHOD == TIMER_RDTSCP
  // 手動預熱，避免第一次調用 elapsed_ns() 時的額外延遲
  Timer::warmup();
#endif
  timer.timeReset();
  for (int t = 0; t < 2; ++t) {
    for (size_t i = 0; i < sampleNum; ++i) {
      // 搜尋時間量測 (PT)
      tree.search(packets[i]);
      timer.timeReset();
      tree.search(packets[i]);
      PT_y(i) = static_cast<double>(timer.elapsed_ns());
      // PT_y(i) += static_cast<double>( timer.elapsed_ns());
    }
  }
  // for (size_t i = 0; i < sampleNum; ++i) {
  //   PT_y(i) = (PT_y(i) / 2.0);
  // }
  auto [mean_PT, median_PT, per75_PT, per95_PT, per99_PT] =
      printStatistics("PT", PT_y);
  slow_time = per95_PT;
  slow_Packets.clear();
  for (size_t i = 0; i < sampleNum; ++i) {
    if (PT_y(i) >= slow_time) slow_Packets.emplace_back(i);
  }
  slow_Packets.shrink_to_fit();
}
void DBT_Object::build_dbt() {
  sampleNum = packets.size();
  if (0 >= sampleNum) std::cerr << "\nDBT build WRONG(0 >= sampleNum)!!\n";
  DBT::TOP_K = top_k;
  DBT::END_BOUND = end_bound;
  DBT::C_BOUND = c_bound;
  DBT::BINTH = binth;
  DBT::DBTable dbt(rules, DBT::BINTH);
  dbt.construct();  // 調用 DBTable 的構建
  // dbt.mem();
  Eigen::VectorXd DBT_y(sampleNum);
  Timer timer;
#if TIMER_METHOD == TIMER_RDTSCP
  // 手動預熱，避免第一次調用 elapsed_ns() 時的額外延遲
  Timer::warmup();
#endif
  timer.timeReset();
  for (int t = 0; t < 2; ++t) {
    for (size_t i = 0; i < sampleNum; ++i) {
      // 搜尋時間量測 (DBT)
      dbt.search(packets[i]);
      timer.timeReset();
      dbt.search(packets[i]);
      DBT_y(i) = static_cast<double>(timer.elapsed_ns());
      // DBT_y(i) += static_cast<double>( timer.elapsed_ns());
    }
  }
  // for (size_t i = 0; i < sampleNum; ++i) {
  //   DBT_y(i) = (DBT_y(i) / 2.0);
  // }
  auto [mean_DBT, median_DBT, per75_DBT, per95_DBT, per99_DBT] =
      printStatistics("DBT", DBT_y);
  slow_time = per95_DBT;
  slow_Packets.clear();
  for (size_t i = 0; i < sampleNum; ++i) {
    if (DBT_y(i) >= slow_time) slow_Packets.emplace_back(i);
  }
  slow_Packets.shrink_to_fit();
}
void DT_Object::build_dt() {
  sampleNum = packets.size();
  if (0 >= sampleNum) std::cerr << "\nDT build WRONG(0 >= sampleNum)!!\n";
  if (is_prefix_5d == true) rules = RulesPortPrefix(rules, true);
  DynamicTuple dynamictuple;
  dynamictuple.threshold = threshold;
  if (0 < dynamictuple.threshold) dynamictuple.use_port_hash_table = true;
  dynamictuple.Create(rules, true);
  Eigen::VectorXd DT_y(sampleNum);
  Timer timer;
#if TIMER_METHOD == TIMER_RDTSCP
  // 手動預熱，避免第一次調用 elapsed_ns() 時的額外延遲
  Timer::warmup();
#endif
  timer.timeReset();
  for (int t = 0; t < 2; ++t) {
    for (size_t i = 0; i < sampleNum; ++i) {
      // 搜尋時間量測 (DT)
      (dynamictuple.Lookup(packets[i], 0));
      timer.timeReset();
      (dynamictuple.Lookup(packets[i], 0));
      DT_y(i) = static_cast<double>(timer.elapsed_ns());
      // DT_y(i) += static_cast<double>(timer.elapsed_ns());
    }
  }
  // for (size_t i = 0; i < sampleNum; ++i) {
  //   DT_y(i) = (DT_y(i) / 2.0);
  // }
  auto [mean_DT, median_DT, per75_DT, per95_DT, per99_DT] =
      printStatistics("DT", DT_y);
  slow_time = per95_DT;
  slow_Packets.clear();
  for (size_t i = 0; i < sampleNum; ++i) {
    if (DT_y(i) >= slow_time) slow_Packets.emplace_back(i);
  }
  slow_Packets.shrink_to_fit();
  dynamictuple.Free(false);  // JIA
}
// PT 計算
double compute_pt(const PT_Object& pt_obj) {
  double score = pt_obj.slow_time;
  cout << "PT score: " << score << endl;
  return score;
}
// PT 和 DBT 的交集大小作為互補性評分（越小越好）
double compute_complementarity(const PT_Object& pt_obj,
                               const DBT_Object& dbt_obj) {
  auto pt_slow = pt_obj.get_slow_Packets();
  auto dbt_slow = dbt_obj.get_slow_Packets();
  vector<int> intersection = getIntersection(pt_slow, dbt_slow);
  double score = static_cast<double>(intersection.size());
  cout << "PT ∩ DBT slow packets count (lower is better): " << score << endl;
  return score;
}
// PT, DBT 和 DT 三者交集大小作為整體互補性評分（越小越好）
double compute_group_complementarity(const DT_Object& dt_obj,
                                     const PT_Object& pt_obj,
                                     const DBT_Object& dbt_obj) {
  auto pt_slow = pt_obj.get_slow_Packets();
  auto dbt_slow = dbt_obj.get_slow_Packets();
  auto dt_slow = dt_obj.get_slow_Packets();
  vector<int> diff_first = getIntersection(pt_slow, dbt_slow);
  vector<int> diff_second = getIntersection(diff_first, dt_slow);
  double score = static_cast<double>(diff_second.size());
  cout << "PT ∩ DBT ∩ DT slow packets count (lower is better): " << score
       << endl;
  return score;
}

// RLGym 實現
PT_Object RLGym::create_pt_object(std::vector<uint8_t> set_field,
                                  int set_port) {
  if (0 >= this->KSet_rule.size()) cerr << "\n0 >= KSet_rule size WRONG!!\n";
  auto pt_rules = convertToPTRules(this->KSet_rule);
  auto pt_packets = convertToPTPackets(this->KSet_packets);
  if (set_field.size() == 0) {
    // CacuInfo cacu(pt_rules);
    // cacu.read_fields();
    // set_field = cacu.cacu_best_fields();
    set_field = {4, 0, 1};
    set_port = 1;
  }
  return PT_Object(set_field, set_port, pt_rules, pt_packets);
}

DBT_Object RLGym::create_dbt_object(int binth, double end_bound, int top_k,
                                    int c_bound, const PT_Object& pt_obj) {
  if (0 >= this->KSet_rule.size()) cerr << "\n0 >= KSet_rule size WRONG!!\n";
  auto dbt_rules = convertToDBTRules(this->KSet_rule);
  auto dbt_packets = convertToDBTPackets(this->KSet_packets);

  int adjusted_binth = (binth);
  double adjusted_end_bound = (end_bound);
  int adjusted_top_k = (top_k);
  int adjusted_c_bound = (c_bound);

  // int adjusted_binth = (++binth);
  // double adjusted_end_bound = (0.1 + end_bound);
  // int adjusted_top_k = (++top_k);
  // int adjusted_c_bound = (++c_bound);
  // if (adjusted_binth >= 33) {
  //   adjusted_binth = 32;
  //   cout << "adjusted_binth = 32\n";
  // }
  // if (adjusted_end_bound >= 1.0) {
  //   adjusted_end_bound = 0.9;
  //   cout << "adjusted_end_bound = 0.9\n";
  // }
  // if (adjusted_top_k >= 9) {
  //   adjusted_top_k = 8;
  //   cout << "adjusted_top_k = 8\n";
  // }
  // if (adjusted_c_bound >= 65) {
  //   adjusted_c_bound = 64;
  //   cout << "adjusted_c_bound = 64\n";
  // }
  return DBT_Object(adjusted_binth, adjusted_end_bound, adjusted_top_k,
                    adjusted_c_bound, dbt_rules, dbt_packets);
}

DT_Object RLGym::create_dt_object(int threshold, bool is_prefix_5d,
                                  const PT_Object& pt_obj,
                                  const DBT_Object& dbt_obj) {
  if (0 >= this->KSet_rule.size()) cerr << "\n0 >= KSet_rule size WRONG!!\n";
  auto dt_rules = convertRules_KSetToDTMT(this->KSet_rule);
  auto dt_packets = convertPackets_KSet2DTMT(this->KSet_packets);

  int adjusted_threshold = (threshold);
  bool adjusted_is_prefix_5d = (is_prefix_5d);

  // int adjusted_threshold = (++threshold);
  // bool adjusted_is_prefix_5d = bool(!is_prefix_5d);
  // if (adjusted_threshold >= 33) {
  //   adjusted_threshold = 32;
  //   cout << "adjusted_threshold = 32\n";
  // }
  return DT_Object(adjusted_threshold, adjusted_is_prefix_5d, dt_rules,
                   dt_packets);
}

double RLGym::evaluate_pt(const PT_Object& pt_obj) {
  return compute_pt(pt_obj);
}

double RLGym::evaluate_dbt(const PT_Object& pt_obj, const DBT_Object& dbt_obj) {
  return compute_complementarity(pt_obj, dbt_obj);
}

double RLGym::evaluate_dt(const PT_Object& pt_obj, const DBT_Object& dbt_obj,
                          const DT_Object& dt_obj) {
  return compute_group_complementarity(dt_obj, pt_obj, dbt_obj);
}
void RLGym::load_KSet_rule_packets(const char* rule_filename,
                                   const char* packet_filename) {
  InputFile inputFile;
  inputFile.loadRule(this->KSet_rule, rule_filename);
  inputFile.loadPacket(this->KSet_packets, packet_filename);
  if (0 >= this->KSet_rule.size()) cerr << "\n0 >= KSet_rule size WRONG!!\n";
  if (0 >= this->KSet_packets.size())
    cerr << "\n0 >= KSet_packets size WRONG!!\n";
}

PT_Object RLGym::create_pt_first() {
  if (0 >= this->KSet_rule.size()) cerr << "\n0 >= KSet_rule size WRONG!!\n";
  auto pt_rules = convertToPTRules(this->KSet_rule);
  auto pt_packets = convertToPTPackets(this->KSet_packets);
  std::vector<uint8_t> set_field;
  int set_port;
  if (set_field.size() == 0) {
    // CacuInfo cacu(pt_rules);
    // cacu.read_fields();
    // set_field = cacu.cacu_best_fields();
    set_field = {4, 0, 1};
    set_port = 1;
  }
  return PT_Object(set_field, set_port, pt_rules, pt_packets);
}

DBT_Object RLGym::create_dbt_first() {
  int binth = 4;
  double end_bound = 0.8;
  int top_k = 4;
  int c_bound = 32;
  if (0 >= this->KSet_rule.size()) cerr << "\n0 >= KSet_rule size WRONG!!\n";
  auto dbt_rules = convertToDBTRules(this->KSet_rule);
  auto dbt_packets = convertToDBTPackets(this->KSet_packets);
  return DBT_Object(binth, end_bound, top_k, c_bound, dbt_rules, dbt_packets);
}

DT_Object RLGym::create_dt_first() {
  int threshold = 7;
  bool is_prefix_5d = false;
  if (0 >= this->KSet_rule.size()) cerr << "\n0 >= KSet_rule size WRONG!!\n";
  auto dt_rules = convertRules_KSetToDTMT(this->KSet_rule);
  auto dt_packets = convertPackets_KSet2DTMT(this->KSet_packets);
  return DT_Object(threshold, is_prefix_5d, dt_rules, dt_packets);
}

// pybind11 綁定
PYBIND11_MODULE(rl_gym, m) {
  py::class_<PT_Object>(m, "PT_Object")
      //.def(py::init<>())
      .def(py::init<std::vector<uint8_t>, int, std::vector<PT_Rule>&,
                    const std::vector<PT_Packet>&>())
      .def("get_set_port", &PT_Object::get_set_port)
      .def("get_set_field", &PT_Object::get_set_field)
      .def("set_all_params", &PT_Object::set_all_params);

  py::class_<DBT_Object>(m, "DBT_Object")
      //.def(py::init<>())
      .def(py::init<int, double, int, int, std::vector<DBT::Rule>&,
                    const std::vector<DBT::Packet>&>())
      .def("get_binth", &DBT_Object::get_binth)
      .def("get_end_bound", &DBT_Object::get_end_bound)
      .def("get_top_k", &DBT_Object::get_top_k)
      .def("get_c_bound", &DBT_Object::get_c_bound)
      .def("set_binth", &DBT_Object::set_binth)
      .def("set_end_bound", &DBT_Object::set_end_bound)
      .def("set_top_k", &DBT_Object::set_top_k)
      .def("set_c_bound", &DBT_Object::set_c_bound);

  py::class_<DT_Object>(m, "DT_Object")
      //.def(py::init<>())
      .def(py::init<int, bool, std::vector<Rule_DT_MT*>&,
                    const std::vector<Trace*>&>())
      .def("get_threshold", &DT_Object::get_threshold)
      .def("get_is_prefix_5d", &DT_Object::get_is_prefix_5d)
      .def("set_th", &DT_Object::set_th)
      .def("set_is_prefix_5d", &DT_Object::set_is_prefix_5d);

  py::class_<RLGym>(m, "RLGym")
      .def(py::init<>())
      .def("load_KSet_rule_packets", &RLGym::load_KSet_rule_packets)
      .def("create_pt_object", &RLGym::create_pt_object)
      .def("create_dbt_object", &RLGym::create_dbt_object)
      .def("create_dt_object", &RLGym::create_dt_object)
      .def("evaluate_pt", &RLGym::evaluate_pt)
      .def("evaluate_dbt", &RLGym::evaluate_dbt)
      .def("evaluate_dt", &RLGym::evaluate_dt)
      .def("create_pt_first", &RLGym::create_pt_first)
      .def("create_dbt_first", &RLGym::create_dbt_first)
      .def("create_dt_first", &RLGym::create_dt_first);
}
