#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "KSet.hpp"
#include "LinearRegressionModel.hpp"
#include "basis.hpp"
#include "input.hpp"
#include "inputFile_test.hpp"
#include "linearSearch_test.hpp"
///////// PT //////////
#include "PT_tree.hpp"
///////// PT //////////
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

using namespace std;

#ifndef TIMER_METHOD
#define TIMER_METHOD TIMER_RDTSCP
#endif

#define EIGEN_NO_DEBUG  // 關閉 Eigen assert
#define EIGEN_UNROLL_LOOP_LIMIT 256
#define CACHE
#define PERLOOKUPTIME_MODEL
#define PERLOOKUPTIME_INDIVIDUAL
// #define PERLOOKUPTIME_BLOOM
// #define BLOOM
///////// bloomFilter /////////
#ifdef BLOOM
#include "bloomFilter.hpp"
#endif
///////// bloomFilter /////////
constexpr const char *LoadRule_test_path = "./INFO/loadRule_test.txt";
constexpr const char *LoadPacket_test_path = "./INFO/loadPacket_test.txt";
// 靜態成員初始化
struct option CommandLineParser::long_options[] = {
    {"ruleset", required_argument, NULL, 'r'},
    {"trace", required_argument, NULL, 'p'},
    {"test", no_argument, NULL, 't'},
    {"classification", no_argument, NULL, 's'},
    {"update", no_argument, NULL, 'u'},
    {"help", no_argument, NULL, 'h'},
    {0, 0, 0, 0}  // 結束標記
};

constexpr int threshold = 10;  // linear search threshold

// for KSet
constexpr int pre_K = 16;
int SetBits[3] = {8, 8, 8};

// JIA
int max_pri_set[5] = {-1, -1, -1, -1, -1};
int &kset_match_pri = max_pri_set[4];
// int max_pri_set[4] = {-1, -1, -1, -1};
// int kset_match_pri = -1;

void anaK(const size_t number_rule, const vector<Rule> &rule, int *usedbits,
          vector<Rule> set[4], int k) {
  uint32_t tmpKey;
  int hash;

  int pre_seg[4] = {0};
  int max[3] = {-1};

  // compute rules in each set
  for (size_t i = 0; i < number_rule; ++i) {
    // Set 0
    if ((rule[i].prefix_length[0] >= k) && (rule[i].prefix_length[1] >= k)) {
      if (rule[i].priority > max_pri_set[0]) max_pri_set[0] = rule[i].priority;
      ++pre_seg[0];
    }
    // Set 1
    else if ((rule[i].prefix_length[0] >= k) &&
             (rule[i].prefix_length[1] < k)) {
      if (rule[i].priority > max_pri_set[1]) max_pri_set[1] = rule[i].priority;
      ++pre_seg[1];
    }
    // Set 2
    else if ((rule[i].prefix_length[0] < k) &&
             (rule[i].prefix_length[1] >= k)) {
      if (rule[i].priority > max_pri_set[2]) max_pri_set[2] = rule[i].priority;
      ++pre_seg[2];
    }
    // Set 3
    else {
      if (rule[i].priority > max_pri_set[3]) max_pri_set[3] = rule[i].priority;
      ++pre_seg[3];
    }
  }
  cout << ("\n================KSet Precompute=============\n");
  cout << "Set 0: " << pre_seg[0] << ", Set 1: " << pre_seg[1]
       << ", Set 2: " << pre_seg[2] << ", Set 3: " << pre_seg[3] << endl;

  cout << "max_pri[0]: " << max_pri_set[0] << ", max_pri[1]: " << max_pri_set[1]
       << ", max_pri[2]: " << max_pri_set[2]
       << ", max_pri[3]: " << max_pri_set[3] << endl;
  // compute used bits
  for (size_t i = 0; i < 3; ++i) {
    if (pre_seg[i] == 0) continue;
    if (log2(pre_seg[i]) > 8 && log2(pre_seg[i]) < 16) {
      usedbits[i] = log2(pre_seg[i]);
    } else if (log2(pre_seg[i]) > 16) {
      usedbits[i] = 16;
    } else {
      usedbits[i] = 8;
    }
  }
  cout << "Set 0: " << usedbits[0] << " bits, "
       << "Set 1: " << usedbits[1] << " bits, "
       << "Set 2: " << usedbits[2] << " bits" << endl;
  for (size_t i = 0; i < 4; ++i) {
    max_pri_set[i] = -1;
  }
  // put rules in each set and analyze
  anaSet Set[3];
  for (size_t i = 0; i < 3; ++i) {
    Set[i].tablesize = pow(2, usedbits[i]);
    Set[i].seg = new int[Set[i].tablesize];
  }

  for (size_t i = 0; i < number_rule; ++i) {
    // Set 0
    if ((rule[i].prefix_length[0] >= usedbits[0]) &&
        (rule[i].prefix_length[1] >= usedbits[0])) {
      set[0].push_back(rule[i]);
      if (rule[i].priority > max_pri_set[0]) max_pri_set[0] = rule[i].priority;
      // compute orig_key
      tmpKey = rule[i].range[0][0] >> (32 - usedbits[0]);
      tmpKey <<= usedbits[0];
      tmpKey += (rule[i].range[1][0] >> (32 - usedbits[0]));
      // compute hash to locate seg
      hash = hashSet0(tmpKey, usedbits[0]);

      if (Set[0].seg[hash] == 0) {  // empty
        Set[0].non_empty_seg++;
      }
      Set[0].seg[hash]++;
    }
    // Set 1
    else if (rule[i].prefix_length[0] >= usedbits[1]) {
      set[1].push_back(rule[i]);
      if (rule[i].priority > max_pri_set[1]) max_pri_set[1] = rule[i].priority;
      tmpKey = rule[i].range[0][0] >> (32 - usedbits[1]);

      if (Set[1].seg[tmpKey] == 0) {  // empty
        ++Set[1].non_empty_seg;
      }
      ++Set[1].seg[tmpKey];
    }
    // Set 2
    else if (rule[i].prefix_length[1] >= usedbits[2]) {
      set[2].push_back(rule[i]);
      if (rule[i].priority > max_pri_set[2]) max_pri_set[2] = rule[i].priority;
      tmpKey = rule[i].range[1][0] >> (32 - usedbits[2]);

      if (Set[2].seg[tmpKey] == 0) {  // empty
        ++Set[2].non_empty_seg;
      }
      ++Set[2].seg[tmpKey];
    }
    // Set 3
    else {
      set[3].push_back(rule[i]);
      if (rule[i].priority > max_pri_set[3]) max_pri_set[3] = rule[i].priority;
    }
  }

  // max segment
  for (size_t i = 0; i < 3; ++i) {
    for (size_t j = 0; j < Set[i].tablesize; ++j) {
      if (max[i] < Set[i].seg[j]) max[i] = Set[i].seg[j];
    }
  }

  cout << ("================Compute=============\n");
  cout << "Set 0: " << set[0].size() << ", Set 1: " << set[1].size()
       << ", Set 2: " << set[2].size() << ", Set 3: " << set[3].size() << endl;

  cout << "max_pri[0]: " << max_pri_set[0] << ", max_pri[1]: " << max_pri_set[1]
       << ", max_pri[2]: " << max_pri_set[2]
       << ", max_pri[3]: " << max_pri_set[3] << endl;

  cout << "non-empty_seg[0] = " << Set[0].non_empty_seg
       << ", non-empty_seg[1] = " << Set[1].non_empty_seg
       << ", non-empty_seg[2] = " << Set[2].non_empty_seg << endl;

  cout << fixed << setprecision(3)  // 設定浮點數輸出精度為 3 位小數
       << "AVG[0]: " << (float)set[0].size() / Set[0].non_empty_seg
       << ", AVG[1]: " << (float)set[1].size() / Set[1].non_empty_seg
       << ", AVG[2]: " << (float)set[2].size() / Set[2].non_empty_seg << endl;

  cout << "MAX[0]: " << max[0] << ", MAX[1]: " << max[1]
       << ", MAX[2]: " << max[2] << endl;
}
/////////////////
// ===============================
// Convert Packet -> PT_Packet
// ===============================
vector<PT_Packet> convertToPTPackets(const vector<Packet> &packets) {
  vector<PT_Packet> pt_packets;

  for (const auto &pkt : packets) {
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
vector<Packet> convertFromPTPackets(const vector<PT_Packet> &pt_packets) {
  vector<Packet> packets;

  for (const auto &pt : pt_packets) {
    uint32_t sip, dip;

    // 還原小端序 IP
    bytes_allocate_rev(pt.source_ip, sip);
    bytes_allocate_rev(pt.destination_ip, dip);
    Packet pkt = {sip,         dip, pt.source_port, pt.destination_port,
                  pt.protocol, 0};

    packets.emplace_back(pkt);
  }
  packets.shrink_to_fit();
  return packets;
}

// ===============================
// Convert Rule -> PT_Rule
// ===============================
vector<PT_Rule> convertToPTRules(const vector<Rule> &rules) {
  vector<PT_Rule> pt_rules;

  for (const auto &r : rules) {
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
vector<Rule> convertFromPTRules(const vector<PT_Rule> &pt_rules) {
  vector<Rule> rules;

  for (const auto &pt : pt_rules) {
    Rule r{};
    r.pri = pt.pri;

    // Protocol
    if (pt.protocol[0] == 0xFF) {
      r.range[4][0] = pt.protocol[1];
      r.range[4][1] = pt.protocol[1];
    } else {
      r.range[4][0] = 0;
      r.range[4][1] = 255;
    }

    // IP
    uint32_t sip, dip;
    bytes_allocate_rev(pt.source_ip, sip);
    bytes_allocate_rev(pt.destination_ip, dip);

    r.range[0][0] = r.range[0][1] = sip;
    r.range[1][0] = r.range[1][1] = dip;

    r.prefix_length[0] = pt.source_mask;
    r.prefix_length[1] = pt.destination_mask;

    // Ports
    r.range[2][0] = pt.source_port[0];
    r.range[2][1] = pt.source_port[1];
    r.range[3][0] = pt.destination_port[0];
    r.range[3][1] = pt.destination_port[1];

    rules.emplace_back(r);
  }
  rules.shrink_to_fit();
  return rules;
}

vector<DBT::Packet> convertToDBTPackets(const vector<Packet> &packets) {
  vector<DBT::Packet> dbt_packets;

  for (const Packet &p : packets) {
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
vector<Packet> convertFromDBTPackets(const vector<DBT::Packet> &dbt_packets) {
  vector<Packet> packets;

  for (const auto &dbt_p : dbt_packets) {
    Packet pkt = {dbt_p.ip.i_32.sip, dbt_p.ip.i_32.dip, dbt_p.Port[0],
                  dbt_p.Port[1],     dbt_p.protocol,    0};
    packets.emplace_back(pkt);
  }
  packets.shrink_to_fit();
  return packets;
}

vector<DBT::Rule> convertToDBTRules(const vector<Rule> &rules) {
  vector<DBT::Rule> dbt_rules;

  for (const Rule &r : rules) {
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
vector<Rule> convertFromDBTRules(const vector<DBT::Rule> &dbt_rules) {
  vector<Rule> rules;

  for (const auto &dbt_r : dbt_rules) {
    Rule r{};
    r.pri = dbt_r.pri;

    // Protocol
    if (dbt_r.protocol.mask == 0xFF) {
      r.range[4][0] = dbt_r.protocol.val;
      r.range[4][1] = dbt_r.protocol.val;
    } else {
      r.range[4][0] = 0;
      r.range[4][1] = 255;
    }

    // IP
    r.range[0][0] = r.range[0][1] = dbt_r.ip.i_32.sip;
    r.range[1][0] = r.range[1][1] = dbt_r.ip.i_32.dip;

    r.prefix_length[0] = dbt_r.sip_length;
    r.prefix_length[1] = dbt_r.dip_length;

    // Ports
    r.range[2][0] = dbt_r.Port[0][0];
    r.range[2][1] = dbt_r.Port[0][1];
    r.range[3][0] = dbt_r.Port[1][0];
    r.range[3][1] = dbt_r.Port[1][1];

    rules.emplace_back(r);
  }
  rules.shrink_to_fit();
  return rules;
}

void warmup_PT(PT::PTtree &tree, const std::vector<PT::PT_Packet> &PT_packets,
               const size_t packetNum) {
  for (int i = 0; i < packetNum; ++i) {
    tree.search(PT_packets[i]);
  }
}
void warmup_DBT(DBT::DBTable &dbt, const std::vector<DBT::Packet> &DBT_packets,
                const size_t packetNum) {
  for (int i = 0; i < packetNum; ++i) {
    dbt.search(DBT_packets[i]);
  }
}
void warmup_KSet(vector<KSet> &set, const std::vector<Packet> &packets,
                 const size_t packetNum, const int num_set[]) {
  if (max_pri_set[1] < max_pri_set[2]) max_pri_set[1] = max_pri_set[2];
  for (int i = 0; i < packetNum; ++i) {
    if (num_set[0] > 0) kset_match_pri = set[0].ClassifyAPacket(packets[i]);
    if (kset_match_pri < max_pri_set[1] && num_set[1] > 0)
      kset_match_pri = max(kset_match_pri, set[1].ClassifyAPacket(packets[i]));
    if (kset_match_pri < max_pri_set[2] && num_set[2] > 0)
      kset_match_pri = max(kset_match_pri, set[2].ClassifyAPacket(packets[i]));
    if (kset_match_pri < max_pri_set[3] && num_set[3] > 0)
      kset_match_pri = max(kset_match_pri, set[3].ClassifyAPacket(packets[i]));
  }
}
/////////////////
int main(int argc, char *argv[]) {
  CommandLineParser parser;
  parser.parseArguments(argc, argv);
  vector<Rule> rule;
  vector<Packet> packets;
  InputFile inputFile;
  InputFile_test inputFile_test;
  Timer timer;
  constexpr int trials = 5;  // run 5 times circularly

  inputFile.loadRule(rule, parser.getRulesetFile());
  inputFile.loadPacket(packets, parser.getTraceFile());
  // if (parser.isTestMode()) {
  //   Timer timerTest;
  //   timerTest.timeReset();
  //   inputFile_test.loadRule_test(rule, LoadRule_test_path);
  //   cout << "Input rule test time(ns): " << timerTest.elapsed_ns() << "\n";

  //   timerTest.timeReset();
  //   inputFile_test.loadPacket_test(packets, LoadPacket_test_path);
  //   cout << "Input packet test time(ns): " << timerTest.elapsed_ns() << "\n";
  // }
  const size_t number_rule = rule.size();
  cout << "The number of rules = " << number_rule << "\n";
  const size_t packetNum = packets.size();
  cout << "The number of packets = " << packetNum << "\n";
  unsigned long long PT_search_time = 0, _PT_search_time = 0;
  unsigned long long DBT_search_time = 0, _DBT_search_time = 0;
  unsigned long long KSet_search_time = 0, _KSet_search_time = 0;
  int PT_match_id = 0;
  uint32_t DBT_match_id = 0;

  double mean_PT = 0, median_PT = 0, per25_PT = 0, per75_PT = 0, per95_PT = 0,
         per99_PT = 0, mean_DBT = 0, median_DBT = 0, per25_DBT = 0,
         per75_DBT = 0, per95_DBT = 0, per99_DBT = 0, mean_KSet = 0,
         median_KSet = 0, per25_KSet = 0, per75_KSet = 0, per95_KSet = 0,
         per99_KSet = 0;

  /*************************************************************************/
  ///////// Construct /////////
  //// PT ////
  auto PT_packets = convertToPTPackets(packets);
  auto PT_rules = convertToPTRules(rule);

  setmaskHash();

  // search config
  vector<uint8_t> set_field = parser.getField();
  int set_port = parser.getPort();
  if (set_field.size() == 0) {
    timer.timeReset();
    CacuInfo cacu(PT_rules);
    cacu.read_fields();
    set_field = cacu.cacu_best_fields();
    set_port = 1;

    cout << "\nPT search config time: " << timer.elapsed_s() << " s" << endl;
  }
  for (int i = 0; i < set_field.size(); ++i)
    cout << static_cast<int>(set_field[i]) << " ";
  PTtree tree(set_field, set_port);

  // insert
  // PT construct
  cout << ("\n**************** Construction(PT) ****************\n");
  cout << "\nStart build for single thread...\n|- Using fields:     ";
  for (unsigned int x : set_field) cout << x << ",";
  cout << set_port << endl;
  timer.timeReset();
  for (auto &&r : PT_rules) {
    tree.insert(r);
  }
  cout << "|- Construct time:   " << timer.elapsed_ns() << "ns\n";
  cout << "|- tree.totalNodes: " << tree.totalNodes << endl;

  cout << "|- Memory footprint: " << (double)tree.mem() / 1024.0 / 1024.0
       << "MB\n";
  //// PT ////

  //// DBT ////
  auto DBT_packets = convertToDBTPackets(packets);
  auto DBT_rules = convertToDBTRules(rule);

  cout << "binth=" << DBT::BINTH << " th_b=" << DBT::END_BOUND
       << " K=" << DBT::TOP_K << " th_c=" << DBT::C_BOUND << endl
       << endl;
  DBT::DBTable dbt(DBT_rules, DBT::BINTH);
  // DBT construct
  cout << ("**************** Construction(DBT) ****************\n");
  timer.timeReset();
  dbt.construct();
  cout << "Construction Time: " << timer.elapsed_ns() << " ns\n";
  dbt.mem();
  //// DBT ////

  //// KSet ////
  vector<Rule> set_4[4];
  anaK(number_rule, rule, SetBits, set_4, pre_K);

  int num_set[4] = {0};
  for (size_t i = 0; i < 4; ++i) {
    num_set[i] = set_4[i].size();
  }
  vector<KSet> set = {{0, set_4[0], SetBits[0]},
                      {1, set_4[1], SetBits[1]},
                      {2, set_4[2], SetBits[2]},
                      {3, set_4[3], 0}};

  // ///// OLD VERSION
  // KSet set0(0, set_4[0], SetBits[0]);
  // KSet set1(1, set_4[1], SetBits[1]);
  // KSet set2(2, set_4[2], SetBits[2]);
  // KSet set3(3, set_4[3], 0);
  // ///// OLD VERSION

  KSet &set0 = set[0];
  KSet &set1 = set[1];
  KSet &set2 = set[2];
  KSet &set3 = set[3];
  // KSet construct
  cout << ("**************** Construction(KSet) ****************\n");
  timer.timeReset();
  if (num_set[0] > 0) {
    set0.ConstructClassifier(set_4[0]);
  }
  if (num_set[1] > 0) {
    set1.ConstructClassifier(set_4[1]);
  }
  if (num_set[2] > 0) {
    set2.ConstructClassifier(set_4[2]);
  }
  if (num_set[3] > 0) {
    set3.ConstructClassifier(set_4[3]);
  }
  cout << "\tConstruction time: " << timer.elapsed_ns() << " ns\n";

  // print memory
  set.shrink_to_fit();
  set0.prints();
  set1.prints();
  set2.prints();
  set3.prints();

  //// KSet ////
  ///////// Construct /////////
  /*************************************************************************/

  if (parser.isSearchMode()) {
    unsigned long long Total_search_time = 0, _Total_search_time = 0;
    unsigned long long Total_predict_time = 0, _Total_predict_time = 0;
    /*************************************************************************/
    ///////// Model /////////
    {
      cout
          << ("\n**************** Build(Model 3-D and 11-D) "
              "****************\n");
      cout << "\tThe number of packet in the trace file = " << packetNum
           << "\n";
      cout << "\tTotal packets run " << trials
           << " times circularly: " << packetNum * trials << "\n";

      ///////// train ////////
      // 三維模型
      Eigen::MatrixXd X3(packetNum, 3);  // 3 features
      Eigen::VectorXd PT_model_3(3);
      Eigen::VectorXd DBT_model_3(3);
      Eigen::VectorXd KSet_model_3(3);

      // 11維模型
      Eigen::MatrixXd X11(packetNum, 11);  // 11 features
      Eigen::VectorXd PT_model_11(11);
      Eigen::VectorXd DBT_model_11(11);
      Eigen::VectorXd KSet_model_11(11);

      // y 向量共用 (3, 11)
      Eigen::VectorXd PT_y(packetNum);
      Eigen::VectorXd DBT_y(packetNum);
      Eigen::VectorXd KSet_y(packetNum);

      // double x_source_ip = 0, x_destination_ip = 0;
      double x_source_port = 0, x_destination_port = 0, x_protocol = 0;
      double x_source_ip_0 = 0, x_source_ip_1 = 0, x_source_ip_2 = 0,
             x_source_ip_3 = 0;
      double x_destination_ip_0 = 0, x_destination_ip_1 = 0,
             x_destination_ip_2 = 0, x_destination_ip_3 = 0;

      for (int t = 0; t < 2; ++t) {
        for (int i = 0; i < packetNum; ++i) {
          // 特徵轉換
          // x_source_ip = ip_to_uint32_be(PT_packets[i].source_ip);
          // x_destination_ip = ip_to_uint32_be(PT_packets[i].destination_ip);
          ////
          float out[4] = {0};
          extract_ip_bytes_to_float(PT_packets[i].source_ip, out);
          x_source_ip_0 = static_cast<double>(out[0]);
          x_source_ip_1 = static_cast<double>(out[1]);
          x_source_ip_2 = static_cast<double>(out[2]);
          x_source_ip_3 = static_cast<double>(out[3]);

          extract_ip_bytes_to_float(PT_packets[i].destination_ip, out);
          x_destination_ip_0 = static_cast<double>(out[0]);
          x_destination_ip_1 = static_cast<double>(out[1]);
          x_destination_ip_2 = static_cast<double>(out[2]);
          x_destination_ip_3 = static_cast<double>(out[3]);
          ////
          x_source_port = static_cast<double>(PT_packets[i].source_port);
          x_destination_port =
              static_cast<double>(PT_packets[i].destination_port);
          x_protocol = static_cast<double>(PT_packets[i].protocol);

// 搜尋時間量測 (PT)
#ifdef CACHE
          PT_match_id = tree.search(PT_packets[i]);
#endif
          timer.timeReset();
          PT_match_id = tree.search(PT_packets[i]);
          _PT_search_time = timer.elapsed_ns();
          PT_y(i) = static_cast<double>(_PT_search_time);
        }
      }
      for (int t = 0; t < 2; ++t) {
        for (int i = 0; i < packetNum; ++i) {
          // 特徵轉換
          // x_source_ip = ip_to_uint32_be(PT_packets[i].source_ip);
          // x_destination_ip = ip_to_uint32_be(PT_packets[i].destination_ip);
          ////
          float out[4] = {0};
          extract_ip_bytes_to_float(PT_packets[i].source_ip, out);
          x_source_ip_0 = static_cast<double>(out[0]);
          x_source_ip_1 = static_cast<double>(out[1]);
          x_source_ip_2 = static_cast<double>(out[2]);
          x_source_ip_3 = static_cast<double>(out[3]);

          extract_ip_bytes_to_float(PT_packets[i].destination_ip, out);
          x_destination_ip_0 = static_cast<double>(out[0]);
          x_destination_ip_1 = static_cast<double>(out[1]);
          x_destination_ip_2 = static_cast<double>(out[2]);
          x_destination_ip_3 = static_cast<double>(out[3]);
          ////
          x_source_port = static_cast<double>(PT_packets[i].source_port);
          x_destination_port =
              static_cast<double>(PT_packets[i].destination_port);
          x_protocol = static_cast<double>(PT_packets[i].protocol);

// 搜尋時間量測 (DBT)
#ifdef CACHE
          DBT_match_id = dbt.search(DBT_packets[i]);
#endif
          timer.timeReset();
          DBT_match_id = dbt.search(DBT_packets[i]);
          _DBT_search_time = timer.elapsed_ns();
          DBT_y(i) = static_cast<double>(_DBT_search_time);
        }
      }
      if (max_pri_set[1] < max_pri_set[2]) max_pri_set[1] = max_pri_set[2];

      for (int i = 0; i < packetNum; ++i) {
        // 特徵轉換
        // x_source_ip = ip_to_uint32_be(PT_packets[i].source_ip);
        // x_destination_ip = ip_to_uint32_be(PT_packets[i].destination_ip);
        ////
        float out[4] = {0};
        extract_ip_bytes_to_float(PT_packets[i].source_ip, out);
        x_source_ip_0 = static_cast<double>(out[0]);
        x_source_ip_1 = static_cast<double>(out[1]);
        x_source_ip_2 = static_cast<double>(out[2]);
        x_source_ip_3 = static_cast<double>(out[3]);

        extract_ip_bytes_to_float(PT_packets[i].destination_ip, out);
        x_destination_ip_0 = static_cast<double>(out[0]);
        x_destination_ip_1 = static_cast<double>(out[1]);
        x_destination_ip_2 = static_cast<double>(out[2]);
        x_destination_ip_3 = static_cast<double>(out[3]);
        ////
        x_source_port = static_cast<double>(PT_packets[i].source_port);
        x_destination_port =
            static_cast<double>(PT_packets[i].destination_port);
        x_protocol = static_cast<double>(PT_packets[i].protocol);

// 搜尋時間量測 (KSet)
#ifdef CACHE
        kset_match_pri = -1;
        if (num_set[0] > 0) kset_match_pri = set0.ClassifyAPacket(packets[i]);
        if (kset_match_pri < max_pri_set[1] && num_set[1] > 0)
          kset_match_pri =
              max(kset_match_pri, set1.ClassifyAPacket(packets[i]));
        if (kset_match_pri < max_pri_set[2] && num_set[2] > 0)
          kset_match_pri =
              max(kset_match_pri, set2.ClassifyAPacket(packets[i]));
        if (kset_match_pri < max_pri_set[3] && num_set[3] > 0)
          kset_match_pri =
              max(kset_match_pri, set3.ClassifyAPacket(packets[i]));
#endif

        timer.timeReset();
        kset_match_pri = -1;
        if (num_set[0] > 0) kset_match_pri = set0.ClassifyAPacket(packets[i]);
        if (kset_match_pri < max_pri_set[1] && num_set[1] > 0)
          kset_match_pri =
              max(kset_match_pri, set1.ClassifyAPacket(packets[i]));
        if (kset_match_pri < max_pri_set[2] && num_set[2] > 0)
          kset_match_pri =
              max(kset_match_pri, set2.ClassifyAPacket(packets[i]));
        if (kset_match_pri < max_pri_set[3] && num_set[3] > 0)
          kset_match_pri =
              max(kset_match_pri, set3.ClassifyAPacket(packets[i]));
        _KSet_search_time = timer.elapsed_ns();
        KSet_y(i) = static_cast<double>(_KSet_search_time);

        // 填入三維特徵
        X3(i, 0) = x_source_ip_0;
        X3(i, 1) = x_source_ip_1;
        X3(i, 2) = x_destination_ip_0;

        ////
        // 填入11維特徵
        X11(i, 0) = x_source_ip_0;
        X11(i, 1) = x_source_ip_1;
        X11(i, 2) = x_source_ip_2;
        X11(i, 3) = x_source_ip_3;
        X11(i, 4) = x_destination_ip_0;
        X11(i, 5) = x_destination_ip_1;
        X11(i, 6) = x_destination_ip_2;
        X11(i, 7) = x_destination_ip_3;
        X11(i, 8) = x_source_port;
        X11(i, 9) = x_destination_port;
        X11(i, 10) = x_protocol;
      }

      cout << "|--- PT_y Mean: " << (mean_PT = computeMean(PT_y)) << endl;
      cout << "|--- DBT_y Mean: " << (mean_DBT = computeMean(DBT_y)) << endl;
      cout << "|--- KSet_y Mean: " << (mean_KSet = computeMean(KSet_y)) << endl;

      cout << "|--- PT_y 25th Percentile: "
           << (per25_PT = computePercentile(PT_y, 0.25))
           << " -|--- PT_y Median: " << (median_PT = computeMedian(PT_y))
           << " -|--- PT_y 75th Percentile: "
           << (per75_PT = computePercentile(PT_y, 0.75)) << endl;
      cout << "|--- PT_y 95th Percentile: "
           << (per95_PT = computePercentile(PT_y, 0.95)) << endl;
      cout << "|--- PT_y 99th Percentile: "
           << (per99_PT = computePercentile(PT_y, 0.99)) << endl;

      cout << "|--- DBT_y 25th Percentile: "
           << (per25_DBT = computePercentile(DBT_y, 0.25))
           << " -|--- DBT_y Median: " << (median_DBT = computeMedian(DBT_y))
           << " -|--- DBT_y 75th Percentile: "
           << (per75_DBT = computePercentile(DBT_y, 0.75)) << endl;
      cout << "|--- DBT_y 95th Percentile: "
           << (per95_DBT = computePercentile(DBT_y, 0.95)) << endl;
      cout << "|--- DBT_y 99th Percentile: "
           << (per99_DBT = computePercentile(DBT_y, 0.99)) << endl;

      cout << "|--- KSet_y 25th Percentile: "
           << (per25_KSet = computePercentile(KSet_y, 0.25))
           << " -|--- KSet_y Median: " << (median_KSet = computeMedian(KSet_y))
           << " -|--- KSet_y 75th Percentile: "
           << (per75_KSet = computePercentile(KSet_y, 0.75)) << endl;
      cout << "|--- KSet_y 95th Percentile: "
           << (per95_KSet = computePercentile(KSet_y, 0.95)) << endl;
      cout << "|--- KSet_y 99th Percentile: "
           << (per99_KSet = computePercentile(KSet_y, 0.99)) << endl;
      std::array<double, 3> long_tail_real_time = {per99_PT, per99_DBT,
                                                   per99_KSet};

      for (int i = 0; i < packetNum; ++i) {
        // 特徵轉換
        // x_source_ip = ip_to_uint32_be(PT_packets[i].source_ip);
        // x_destination_ip = ip_to_uint32_be(PT_packets[i].destination_ip);
        ////
        float out[4] = {0};
        extract_ip_bytes_to_float(PT_packets[i].source_ip, out);
        x_source_ip_0 = static_cast<double>(out[0]);
        x_source_ip_1 = static_cast<double>(out[1]);
        x_source_ip_2 = static_cast<double>(out[2]);
        x_source_ip_3 = static_cast<double>(out[3]);

        extract_ip_bytes_to_float(PT_packets[i].destination_ip, out);
        x_destination_ip_0 = static_cast<double>(out[0]);
        x_destination_ip_1 = static_cast<double>(out[1]);
        x_destination_ip_2 = static_cast<double>(out[2]);
        x_destination_ip_3 = static_cast<double>(out[3]);
        ////
        x_source_port = static_cast<double>(PT_packets[i].source_port);
        x_destination_port =
            static_cast<double>(PT_packets[i].destination_port);
        x_protocol = static_cast<double>(PT_packets[i].protocol);

// 搜尋時間量測 (KSet)
#ifdef CACHE
        kset_match_pri = -1;
        if (num_set[0] > 0) kset_match_pri = set0.ClassifyAPacket(packets[i]);
        if (kset_match_pri < max_pri_set[1] && num_set[1] > 0)
          kset_match_pri =
              max(kset_match_pri, set1.ClassifyAPacket(packets[i]));
        if (kset_match_pri < max_pri_set[2] && num_set[2] > 0)
          kset_match_pri =
              max(kset_match_pri, set2.ClassifyAPacket(packets[i]));
        if (kset_match_pri < max_pri_set[3] && num_set[3] > 0)
          kset_match_pri =
              max(kset_match_pri, set3.ClassifyAPacket(packets[i]));
#endif

        timer.timeReset();
        kset_match_pri = -1;
        if (num_set[0] > 0) kset_match_pri = set0.ClassifyAPacket(packets[i]);
        if (kset_match_pri < max_pri_set[1] && num_set[1] > 0)
          kset_match_pri =
              max(kset_match_pri, set1.ClassifyAPacket(packets[i]));
        if (kset_match_pri < max_pri_set[2] && num_set[2] > 0)
          kset_match_pri =
              max(kset_match_pri, set2.ClassifyAPacket(packets[i]));
        if (kset_match_pri < max_pri_set[3] && num_set[3] > 0)
          kset_match_pri =
              max(kset_match_pri, set3.ClassifyAPacket(packets[i]));
        _KSet_search_time = timer.elapsed_ns();
        KSet_y(i) = static_cast<double>(_KSet_search_time);

        // 填入三維特徵
        X3(i, 0) = x_source_ip_0;
        X3(i, 1) = x_source_ip_1;
        X3(i, 2) = x_destination_ip_0;

        ////
        // 填入11維特徵
        X11(i, 0) = x_source_ip_0;
        X11(i, 1) = x_source_ip_1;
        X11(i, 2) = x_source_ip_2;
        X11(i, 3) = x_source_ip_3;
        X11(i, 4) = x_destination_ip_0;
        X11(i, 5) = x_destination_ip_1;
        X11(i, 6) = x_destination_ip_2;
        X11(i, 7) = x_destination_ip_3;
        X11(i, 8) = x_source_port;
        X11(i, 9) = x_destination_port;
        X11(i, 10) = x_protocol;
      }

      Eigen::VectorXd mean_X3, std_X3;
      // Eigen::VectorXdmean_X5, std_X5;
      Eigen::VectorXd mean_X11, std_X11;

      /* JIA normalizeFeatures */ normalizeFeatures(X3, mean_X3, std_X3);
      /* JIA normalizeFeatures */ normalizeFeatures(X11, mean_X11, std_X11);

      // 模型擬合
      PT_model_3 = linearRegressionFit(X3, PT_y);
      PT_model_11 = linearRegressionFit(X11, PT_y);
      DBT_model_3 = linearRegressionFit(X3, DBT_y);
      DBT_model_11 = linearRegressionFit(X11, DBT_y);
      KSet_model_3 = linearRegressionFit(X3, KSet_y);
      KSet_model_11 = linearRegressionFit(X11, KSet_y);

      // 輸出模型參數
      cout << "\n[PT 3-feature model] (x1, x2, x3):\n"
           << PT_model_3.transpose() << endl;
      cout << "\n[PT 11-feature model] (x1~x11):\n"
           << PT_model_11.transpose() << endl;
      cout << "\n[DBT 3-feature model] (x1, x2, x3):\n"
           << DBT_model_3.transpose() << endl;
      cout << "\n[DBT 11-feature model] (x1~x11):\n"
           << DBT_model_11.transpose() << endl;
      cout << "\n[KSet 3-feature model] (x1, x2, x3):\n"
           << KSet_model_3.transpose() << endl;
      cout << "\n[KSet 11-feature model] (x1~x11):\n"
           << KSet_model_11.transpose() << endl;
      ///////// train ////////

      /////// benchmark ///////
      // evaluateModel(X3 * PT_model_3, PT_y, "PT-3-feature");
      // evaluateModel(X11 * PT_model_11, PT_y, "PT-11-feature");
      // evaluateModel(X3 * DBT_model_3, DBT_y, "DBT-3-feature");
      // evaluateModel(X11 * DBT_model_11, DBT_y, "DBT-11-feature");
      // evaluateModel(X3 * KSet_model_3, KSet_y, "KSet-3-feature");
      // evaluateModel(X11 * KSet_model_11, KSet_y, "KSet-11-feature");
      /////// benchmark ///////

      // 輸出封包預測與實際搜尋時間至結果檔案
      ofstream pt_prediction_3_out("./INFO/PT_prediction_3_result.txt");
      if (!pt_prediction_3_out) {
        cerr << "Cannot open PT_prediction_3_result.txt！" << endl;
        return 1;
      }
      ofstream DBT_prediction_3_out("./INFO/DBT_prediction_3_result.txt");
      if (!DBT_prediction_3_out) {
        cerr << "Cannot open DBT_prediction_3_result.txt！" << endl;
        return 1;
      }
      ofstream kset_prediction_3_out("./INFO/KSet_prediction_3_result.txt");
      if (!kset_prediction_3_out) {
        cerr << "Cannot open KSet_prediction_3_result.txt！" << endl;
        return 1;
      }
      ofstream total_prediction_3_out("./INFO/Total_prediction_3_result.txt");
      if (!total_prediction_3_out) {
        cerr << "Cannot open Total_prediction_3_result.txt！" << endl;
        return 1;
      }
      int model_acc = 0, model_fail = 0;
      int model_longTail = 0;              // 出現 long-tail 的樣本數
      int model_select_longTail = 0;       // 被選中的 min_id 是 long-tail 模型
      std::vector<int> long_tail_indices;  // 出現 long-tail 的樣本索引
      std::vector<int>
          select_long_tail_indices;  // 預測選中 long-tail 模型的索引
      //// 3-D model
      for (int i = 0; i < packetNum; ++i) {
        float out[4] = {0};

        // //  source IP（big-endian 轉 uint32）
        // x_source_ip = ip_to_uint32_be(PT_packets[i].source_ip);
        // //  destination IP（big-endian 轉 uint32）
        // x_destination_ip = ip_to_uint32_be(PT_packets[i].destination_ip);

        ////
        extract_ip_bytes_to_float(PT_packets[i].source_ip, out);
        x_source_ip_0 = static_cast<double>(out[0]);
        x_source_ip_1 = static_cast<double>(out[1]);
        //  x_source_ip_2 = static_cast<double>(out[2]);
        //  x_source_ip_3 = static_cast<double>(out[3]);

        extract_ip_bytes_to_float(PT_packets[i].destination_ip, out);
        x_destination_ip_0 = static_cast<double>(out[0]);
        // x_destination_ip_1 = static_cast<double>(out[1]);
        //  x_destination_ip_2 = static_cast<double>(out[2]);
        //  x_destination_ip_3 = static_cast<double>(out[3]);
        ////

        // source port（轉 uint16）
        // x_source_port = static_cast<double>(PT_packets[i].source_port);
        // x_destination_port =
        // static_cast<double>(PT_packets[i].destination_port); x_protocol =
        // static_cast<double>(PT_packets[i].protocol);

        /* JIA normalizeFeatures */
        double x1_norm_3 = toNormalized(x_source_ip_0, mean_X3[0], std_X3[0]);
        double x2_norm_3 = toNormalized(x_source_ip_1, mean_X3[1], std_X3[1]);
        double x3_norm_3 =
            toNormalized(x_destination_ip_0, mean_X3[2], std_X3[2]);

        //// PT
        double predicted_time_3_pt =
            predict3(PT_model_3, x1_norm_3, x2_norm_3, x3_norm_3);

        /* JIA non-normalizeFeatures */
        // double predicted_time_3_pt = predict3(
        //     PT_model_3, x_source_ip_0, x_source_ip_1, x_destination_ip_0);

        double actual_time = PT_y(i);
        pt_prediction_3_out << "Packet " << i << "\tPredict " << fixed
                            << setprecision(4) << predicted_time_3_pt
                            << "\tRealTime(ns) " << actual_time << endl;
        //// PT

        //// DBT
        double predicted_time_3_dbt =
            predict3(DBT_model_3, x1_norm_3, x2_norm_3, x3_norm_3);

        /* JIA non-normalizeFeatures */
        // double predicted_time_3_dbt = predict3(
        //     DBT_model_3, x_source_ip_0, x_source_ip_1, x_destination_ip_0);

        actual_time = DBT_y(i);
        DBT_prediction_3_out << "Packet " << i << "\tPredict " << fixed
                             << setprecision(4) << predicted_time_3_dbt
                             << "\tRealTime(ns) " << actual_time << endl;
        //// DBT

        //// KSet
        double predicted_time_3_kset =
            predict3(KSet_model_3, x1_norm_3, x2_norm_3, x3_norm_3);

        /* JIA non-normalizeFeatures */
        // double predicted_time_3_kset = predict3(
        //     KSet_model_3, x_source_ip_0, x_source_ip_1, x_destination_ip_0);

        actual_time = KSet_y(i);
        kset_prediction_3_out << "Packet " << i << "\tPredict " << fixed
                              << setprecision(4) << predicted_time_3_kset
                              << "\tRealTime(ns) " << actual_time << endl;
        //// KSet
        //// acc
        auto [min_val, min_id_predict] = get_min_max_time(
            predicted_time_3_pt, predicted_time_3_dbt, predicted_time_3_kset);

        // 建立實際值陣列，方便比較與存取
        std::array<double, 3> real_values = {PT_y(i), DBT_y(i), KSet_y(i)};

        // 找出實際最小值與最大值的模型索引
        int real_min_label =
            std::min_element(real_values.begin(), real_values.end()) -
            real_values.begin();
        int real_max_label =
            std::max_element(real_values.begin(), real_values.end()) -
            real_values.begin();

        // 實際的最小與最大值
        double real_min_val = real_values[real_min_label];
        double real_max_val = real_values[real_max_label];

        // Long-tail 偵測
        std::array<bool, 3> is_tail;
        for (int j = 0; j < 3; ++j) {
          if (real_values[j] > (long_tail_real_time[j]))
            is_tail[j] = true;
          else {
            is_tail[j] = false;
          }
        }

        bool has_long_tail = std::any_of(is_tail.begin(), is_tail.end(),
                                         [](bool b) { return b; });
        bool selected_long_tail = is_tail[min_id_predict];

        // 統計與記錄 long-tail 資訊
        if (has_long_tail) {
          ++model_longTail;
          long_tail_indices.push_back(i);
          if (selected_long_tail) {
            ++model_select_longTail;
            select_long_tail_indices.push_back(i);
          }
        }

        // 輸出 log 訊息
        total_prediction_3_out
            << "PT " << PT_y(i) << "\tDBT " << DBT_y(i) << "\tKSet "
            << KSet_y(i) << "\tmin " << real_min_val << "\treal_min_id "
            << real_min_label << "\tmin_id_predict " << min_id_predict
            << "\tMAX " << real_max_val << "\treal_max_id " << real_max_label
            << "\tTAIL " << (selected_long_tail ? "1" : "0") << std::endl;

        // 預測準確與錯誤統計
        if (min_id_predict == real_min_label) ++model_acc;
        if (min_id_predict == real_max_label) ++model_fail;
        //// acc
      }
      pt_prediction_3_out.close();
      DBT_prediction_3_out.close();
      kset_prediction_3_out.close();
      total_prediction_3_out.close();
      cout << "    model_acc 3: " << model_acc / (packetNum * 1.0) << endl;
      cout << "    model_fail 3: " << model_fail / (packetNum * 1.0) << endl;
      cout << "    model_longTail 3: " << model_longTail / (packetNum * 1.0)
           << endl;
      cout << "    model_select_longTail 3: "
           << model_select_longTail / ((model_longTail + 1e-6) * 1.0) << endl;
      //// 3-D

      // 輸出封包預測與實際搜尋時間至結果檔案
      ofstream pt_prediction_11_out("./INFO/PT_prediction_11_result.txt");
      if (!pt_prediction_11_out) {
        cerr << "Cannot open PT_prediction_11_result.txt！" << endl;
        return 1;
      }
      ofstream DBT_prediction_11_out("./INFO/DBT_prediction_11_result.txt");
      if (!DBT_prediction_11_out) {
        cerr << "Cannot open DBT_prediction_11_result.txt！" << endl;
        return 1;
      }
      ofstream kset_prediction_11_out("./INFO/KSet_prediction_11_result.txt");
      if (!kset_prediction_11_out) {
        cerr << "Cannot open KSet_prediction_11_result.txt！" << endl;
        return 1;
      }
      ofstream total_prediction_11_out("./INFO/Total_prediction_11_result.txt");
      if (!total_prediction_11_out) {
        cerr << "Cannot open Total_prediction_11_result.txt！" << endl;
        return 1;
      }

      model_acc = 0;
      model_fail = 0;
      model_longTail = 0;
      model_select_longTail = 0;
      std::vector<int>().swap(long_tail_indices);
      std::vector<int>().swap(select_long_tail_indices);
      for (int i = 0; i < packetNum; ++i) {
        float out[4] = {0};

        extract_ip_bytes_to_float(PT_packets[i].source_ip, out);
        x_source_ip_0 = static_cast<double>(out[0]);
        x_source_ip_1 = static_cast<double>(out[1]);
        x_source_ip_2 = static_cast<double>(out[2]);
        x_source_ip_3 = static_cast<double>(out[3]);

        extract_ip_bytes_to_float(PT_packets[i].destination_ip, out);
        x_destination_ip_0 = static_cast<double>(out[0]);
        x_destination_ip_1 = static_cast<double>(out[1]);
        x_destination_ip_2 = static_cast<double>(out[2]);
        x_destination_ip_3 = static_cast<double>(out[3]);

        // source port（轉 uint16）
        x_source_port = static_cast<double>(PT_packets[i].source_port);
        x_destination_port =
            static_cast<double>(PT_packets[i].destination_port);
        x_protocol = static_cast<double>(PT_packets[i].protocol);

        //// PT
        /* JIA normalizeFeatures */
        double x1_norm_11 =
            toNormalized(x_source_ip_0, mean_X11[0], std_X11[0]);
        double x2_norm_11 =
            toNormalized(x_source_ip_1, mean_X11[1], std_X11[1]);
        double x3_norm_11 =
            toNormalized(x_source_ip_2, mean_X11[2], std_X11[2]);
        double x4_norm_11 =
            toNormalized(x_source_ip_3, mean_X11[3], std_X11[3]);
        double x5_norm_11 =
            toNormalized(x_destination_ip_0, mean_X11[4], std_X11[4]);
        double x6_norm_11 =
            toNormalized(x_destination_ip_1, mean_X11[5], std_X11[5]);
        double x7_norm_11 =
            toNormalized(x_destination_ip_2, mean_X11[6], std_X11[6]);
        double x8_norm_11 =
            toNormalized(x_destination_ip_3, mean_X11[7], std_X11[7]);
        double x9_norm_11 =
            toNormalized(x_source_port, mean_X11[8], std_X11[8]);
        double x10_norm_11 =
            toNormalized(x_destination_port, mean_X11[9], std_X11[9]);
        double x11_norm_11 =
            toNormalized(x_protocol, mean_X11[10], std_X11[10]);

        double predicted_time_11_pt =
            predict11(PT_model_11, x1_norm_11, x2_norm_11, x3_norm_11,
                      x4_norm_11, x5_norm_11, x6_norm_11, x7_norm_11,
                      x8_norm_11, x9_norm_11, x10_norm_11, x11_norm_11);

        /* JIA non-normalizeFeatures */
        // double predicted_time_11_pt =
        //     predict11(PT_model_11, x_source_ip_0, x_source_ip_1,
        // x_source_ip_2, x_source_ip_3, x_destination_ip_0, x_destination_ip_1,
        //               x_destination_ip_2, x_destination_ip_3, x_source_port,
        //               x_destination_port, x_protocol);

        double actual_time = PT_y(i);
        pt_prediction_11_out << "Packet " << i << fixed << setprecision(4)
                             << "\tPredict " << predicted_time_11_pt
                             << "\tRealTime(ns) " << actual_time << endl;
        //// PT

        //// DBT
        double predicted_time_11_dbt =
            predict11(DBT_model_11, x1_norm_11, x2_norm_11, x3_norm_11,
                      x4_norm_11, x5_norm_11, x6_norm_11, x7_norm_11,
                      x8_norm_11, x9_norm_11, x10_norm_11, x11_norm_11);

        /* JIA non-normalizeFeatures */
        // predicted_time_11_dbt =
        // predict11(DBT_model_11, x_source_ip_0, x_source_ip_1,
        // x_source_ip_2, x_source_ip_3, x_destination_ip_0, x_destination_ip_1,
        // x_destination_ip_2, x_destination_ip_3, x_source_port,
        // x_destination_port, x_protocol);

        actual_time = DBT_y(i);
        DBT_prediction_11_out << "Packet " << i << fixed << setprecision(4)
                              << "\tPredict " << predicted_time_11_dbt
                              << "\tRealTime(ns) " << actual_time << endl;
        //// DBT

        //// KSet
        double predicted_time_11_kset =
            predict11(KSet_model_11, x1_norm_11, x2_norm_11, x3_norm_11,
                      x4_norm_11, x5_norm_11, x6_norm_11, x7_norm_11,
                      x8_norm_11, x9_norm_11, x10_norm_11, x11_norm_11);

        /* JIA non-normalizeFeatures */
        // predicted_time_11_kset =
        //     predict11(KSet_model_11, x_source_ip_0, x_source_ip_1,
        // x_source_ip_2, x_source_ip_3, x_destination_ip_0, x_destination_ip_1,
        //               x_destination_ip_2, x_destination_ip_3, x_source_port,
        //               x_destination_port, x_protocol);

        actual_time = KSet_y(i);
        kset_prediction_11_out << "Packet " << i << fixed << setprecision(4)
                               << "\tPredict " << predicted_time_11_kset
                               << "\tRealTime(ns) " << actual_time << endl;
        //// KSet
        //// acc
        auto [min_val, min_id_predict] =
            get_min_max_time(predicted_time_11_pt, predicted_time_11_dbt,
                             predicted_time_11_kset);

        // 建立實際值陣列，方便比較與存取
        std::array<double, 3> real_values = {PT_y(i), DBT_y(i), KSet_y(i)};

        // 找出實際最小值與最大值的模型索引
        int real_min_label =
            std::min_element(real_values.begin(), real_values.end()) -
            real_values.begin();
        int real_max_label =
            std::max_element(real_values.begin(), real_values.end()) -
            real_values.begin();

        // 實際的最小與最大值
        double real_min_val = real_values[real_min_label];
        double real_max_val = real_values[real_max_label];

        // Long-tail 偵測
        std::array<bool, 3> is_tail;
        for (int j = 0; j < 3; ++j) {
          if (real_values[j] > (long_tail_real_time[j]))
            is_tail[j] = true;
          else {
            is_tail[j] = false;
          }
        }

        bool has_long_tail = std::any_of(is_tail.begin(), is_tail.end(),
                                         [](bool b) { return b; });
        bool selected_long_tail = is_tail[min_id_predict];

        // 統計與記錄 long-tail 資訊
        if (has_long_tail) {
          ++model_longTail;
          long_tail_indices.push_back(i);
          if (selected_long_tail) {
            ++model_select_longTail;
            select_long_tail_indices.push_back(i);
          }
        }

        // 輸出 log 訊息
        total_prediction_11_out
            << "PT " << PT_y(i) << "\tDBT " << DBT_y(i) << "\tKSet "
            << KSet_y(i) << "\tmin " << real_min_val << "\treal_min_id "
            << real_min_label << "\tmin_id_predict " << min_id_predict
            << "\tMAX " << real_max_val << "\treal_max_id " << real_max_label
            << "\tTAIL " << (selected_long_tail ? "1" : "0") << std::endl;

        // 預測準確與錯誤統計
        if (min_id_predict == real_min_label) ++model_acc;
        if (min_id_predict == real_max_label) ++model_fail;
        //// acc
      }
      pt_prediction_11_out.close();
      DBT_prediction_11_out.close();
      kset_prediction_11_out.close();
      total_prediction_11_out.close();
      cout << "    model_acc 11: " << model_acc / (packetNum * 1.0) << endl;
      cout << "    model_fail 11: " << model_fail / (packetNum * 1.0) << endl;
      cout << "    model_longTail 11: " << model_longTail / (packetNum * 1.0)
           << endl;
      cout << "    model_select_longTail 11: "
           << model_select_longTail / ((model_longTail + 1e-6) * 1.0) << endl;
      //// 11-D
      ///////// Model /////////

      ///////// Model Total /////////
      cout << ("\n**************** Classification(TOTAL) ****************\n");
      int model_counter_DBT = 0, model_counter_PT = 0, model_counter_KSet = 0;
      Total_predict_time = 0;
      Total_search_time = 0;
#ifdef PERLOOKUPTIME_MODEL
      Eigen::VectorXd Total_y(packetNum);
      FILE *total_model_3_fp = nullptr;
      total_model_3_fp = fopen("./INFO/Total_model_3_result.txt", "w");
      FILE *total_model_11_fp = nullptr;
      total_model_11_fp = fopen("./INFO/Total_model_11_result.txt", "w");
#endif
      warmup_KSet(set, packets, packetNum, num_set);
      warmup_PT(tree, PT_packets, packetNum);
      warmup_DBT(dbt, DBT_packets, packetNum);
      if (max_pri_set[1] < max_pri_set[2]) max_pri_set[1] = max_pri_set[2];
      for (size_t t = 0; t < trials; ++t) {
        for (int i = 0; i < packetNum; ++i) {
          float out[4] = {0};
          kset_match_pri = -1;

          extract_ip_bytes_to_float(PT_packets[i].source_ip, out);
          x_source_ip_0 = static_cast<double>(out[0]);
          x_source_ip_1 = static_cast<double>(out[1]);
          //  x_source_ip_2 = static_cast<double>(out[2]);
          //  x_source_ip_3 = static_cast<double>(out[3]);

          extract_ip_bytes_to_float(PT_packets[i].destination_ip, out);
          x_destination_ip_0 = static_cast<double>(out[0]);
          // x_destination_ip_1 = static_cast<double>(out[1]);
          //  x_destination_ip_2 = static_cast<double>(out[2]);
          //  x_destination_ip_3 = static_cast<double>(out[3]);
          timer.timeReset();
          /* JIA normalizeFeatures */
          double x1_norm_3 = toNormalized(x_source_ip_0, mean_X3[0], std_X3[0]);
          double x2_norm_3 = toNormalized(x_source_ip_1, mean_X3[1], std_X3[1]);
          double x3_norm_3 =
              toNormalized(x_destination_ip_0, mean_X3[2], std_X3[2]);
          // timer.timeReset();
          //// PT
          double predicted_time_3_pt =
              predict3(PT_model_3, x1_norm_3, x2_norm_3, x3_norm_3);
          //// PT

          //// DBT
          double predicted_time_3_dbt =
              predict3(DBT_model_3, x1_norm_3, x2_norm_3, x3_norm_3);
          //// DBT

          //// KSet
          double predicted_time_3_kset =
              predict3(KSet_model_3, x1_norm_3, x2_norm_3, x3_norm_3);
          //// KSet
          /* JIA normalizeFeatures */

          // /* JIA non-normalizeFeatures */
          // timer.timeReset();
          // //// PT
          // double predicted_time_3_pt = predict3(
          //     PT_model_3, x_source_ip_0, x_source_ip_1, x_destination_ip_0);
          // //// PT

          // //// DBT
          // double predicted_time_3_dbt = predict3(
          //     DBT_model_3, x_source_ip_0, x_source_ip_1, x_destination_ip_0);
          // //// DBT

          // //// KSet
          // double predicted_time_3_kset = predict3(
          //     KSet_model_3, x_source_ip_0, x_source_ip_1,
          //     x_destination_ip_0);
          // //// KSet
          // /* JIA non-normalizeFeatures */

          if ((predicted_time_3_dbt <= predicted_time_3_kset)) {
            if ((predicted_time_3_dbt <= predicted_time_3_pt)) {
              _Total_predict_time = timer.elapsed_ns();
              Total_predict_time += _Total_predict_time;
              timer.timeReset();
              dbt.search(DBT_packets[i]);
              _Total_search_time = timer.elapsed_ns();
              Total_search_time += _Total_search_time;
              ++model_counter_DBT;
            } else {
              _Total_predict_time = timer.elapsed_ns();
              Total_predict_time += _Total_predict_time;
              timer.timeReset();
              tree.search(PT_packets[i]);
              _Total_search_time = timer.elapsed_ns();
              Total_search_time += _Total_search_time;
              ++model_counter_PT;
            }
          } else if ((predicted_time_3_pt <= predicted_time_3_kset)) {
            _Total_predict_time = timer.elapsed_ns();
            Total_predict_time += _Total_predict_time;
            timer.timeReset();
            tree.search(PT_packets[i]);
            _Total_search_time = timer.elapsed_ns();
            Total_search_time += _Total_search_time;
            ++model_counter_PT;
          } else {
            _Total_predict_time = timer.elapsed_ns();
            Total_predict_time += _Total_predict_time;
            timer.timeReset();
            if (num_set[0] > 0)
              kset_match_pri = set0.ClassifyAPacket(packets[i]);
            if (kset_match_pri < max_pri_set[1] && num_set[1] > 0)
              kset_match_pri =
                  max(kset_match_pri, set1.ClassifyAPacket(packets[i]));
            if (kset_match_pri < max_pri_set[2] && num_set[2] > 0)
              kset_match_pri =
                  max(kset_match_pri, set2.ClassifyAPacket(packets[i]));
            if (kset_match_pri < max_pri_set[3] && num_set[3] > 0)
              kset_match_pri =
                  max(kset_match_pri, set3.ClassifyAPacket(packets[i]));
            _Total_search_time = timer.elapsed_ns();
            Total_search_time += _Total_search_time;
            ++model_counter_KSet;
          }
#ifdef PERLOOKUPTIME_MODEL
          Total_y(i) = (_Total_search_time + _Total_predict_time);
#endif
        }
      }
      cout << "|=== AVG predict time(Model-3): "
           << Total_predict_time / (packetNum * trials) << "ns\n";
      cout << "|=== AVG search with predict time(Model-3): "
           << (Total_predict_time + Total_search_time) / (packetNum * trials)
           << "ns\n";
      cout << "|=== PT, DBT, KSET: "
           << (model_counter_PT / trials) / (packetNum * 1.0) << ", "
           << (model_counter_DBT / trials) / (packetNum * 1.0) << ", "
           << (model_counter_KSet / trials) / (packetNum * 1.0) << "\n";
#ifdef PERLOOKUPTIME_MODEL
      for (int i = 0; i < packetNum; ++i) {
        fprintf(total_model_3_fp, "Packet %d \t Time(ns) %f\n", i,
                Total_y(i) / 1.0);
      }
      fclose(total_model_3_fp);
#endif
      model_counter_DBT = 0;
      model_counter_PT = 0;
      model_counter_KSet = 0;
      Total_predict_time = 0;
      Total_search_time = 0;

      warmup_KSet(set, packets, packetNum, num_set);
      warmup_PT(tree, PT_packets, packetNum);
      warmup_DBT(dbt, DBT_packets, packetNum);
      if (max_pri_set[1] < max_pri_set[2]) max_pri_set[1] = max_pri_set[2];
      for (size_t t = 0; t < trials; ++t) {
        for (int i = 0; i < packetNum; ++i) {
          float out[4] = {0};
          kset_match_pri = -1;

          extract_ip_bytes_to_float(PT_packets[i].source_ip, out);
          x_source_ip_0 = static_cast<double>(out[0]);
          x_source_ip_1 = static_cast<double>(out[1]);
          x_source_ip_2 = static_cast<double>(out[2]);
          x_source_ip_3 = static_cast<double>(out[3]);

          extract_ip_bytes_to_float(PT_packets[i].destination_ip, out);
          x_destination_ip_0 = static_cast<double>(out[0]);
          x_destination_ip_1 = static_cast<double>(out[1]);
          x_destination_ip_2 = static_cast<double>(out[2]);
          x_destination_ip_3 = static_cast<double>(out[3]);

          // source port（轉 uint16）
          x_source_port = static_cast<double>(PT_packets[i].source_port);
          x_destination_port =
              static_cast<double>(PT_packets[i].destination_port);
          x_protocol = static_cast<double>(PT_packets[i].protocol);
          timer.timeReset();
          /* JIA normalizeFeatures */
          double x1_norm_11 =
              toNormalized(x_source_ip_0, mean_X11[0], std_X11[0]);
          double x2_norm_11 =
              toNormalized(x_source_ip_1, mean_X11[1], std_X11[1]);
          double x3_norm_11 =
              toNormalized(x_source_ip_2, mean_X11[2], std_X11[2]);
          double x4_norm_11 =
              toNormalized(x_source_ip_3, mean_X11[3], std_X11[3]);
          double x5_norm_11 =
              toNormalized(x_destination_ip_0, mean_X11[4], std_X11[4]);
          double x6_norm_11 =
              toNormalized(x_destination_ip_1, mean_X11[5], std_X11[5]);
          double x7_norm_11 =
              toNormalized(x_destination_ip_2, mean_X11[6], std_X11[6]);
          double x8_norm_11 =
              toNormalized(x_destination_ip_3, mean_X11[7], std_X11[7]);
          double x9_norm_11 =
              toNormalized(x_source_port, mean_X11[8], std_X11[8]);
          double x10_norm_11 =
              toNormalized(x_destination_port, mean_X11[9], std_X11[9]);
          double x11_norm_11 =
              toNormalized(x_protocol, mean_X11[10], std_X11[10]);
          // timer.timeReset();
          //// PT
          double predicted_time_11_pt =
              predict11(PT_model_11, x1_norm_11, x2_norm_11, x3_norm_11,
                        x4_norm_11, x5_norm_11, x6_norm_11, x7_norm_11,
                        x8_norm_11, x9_norm_11, x10_norm_11, x11_norm_11);
          //// PT

          //// DBT
          double predicted_time_11_dbt =
              predict11(DBT_model_11, x1_norm_11, x2_norm_11, x3_norm_11,
                        x4_norm_11, x5_norm_11, x6_norm_11, x7_norm_11,
                        x8_norm_11, x9_norm_11, x10_norm_11, x11_norm_11);
          //// DBT

          //// KSet
          double predicted_time_11_kset =
              predict11(KSet_model_11, x1_norm_11, x2_norm_11, x3_norm_11,
                        x4_norm_11, x5_norm_11, x6_norm_11, x7_norm_11,
                        x8_norm_11, x9_norm_11, x10_norm_11, x11_norm_11);
          //// KSet

          if ((predicted_time_11_dbt <= predicted_time_11_kset)) {
            if ((predicted_time_11_dbt <= predicted_time_11_pt)) {
              _Total_predict_time = timer.elapsed_ns();
              Total_predict_time += _Total_predict_time;
              timer.timeReset();
              dbt.search(DBT_packets[i]);
              _Total_search_time = timer.elapsed_ns();
              Total_search_time += _Total_search_time;
              ++model_counter_DBT;
            } else {
              _Total_predict_time = timer.elapsed_ns();
              Total_predict_time += _Total_predict_time;
              timer.timeReset();
              tree.search(PT_packets[i]);
              _Total_search_time = timer.elapsed_ns();
              Total_search_time += _Total_search_time;
              ++model_counter_PT;
            }
          } else if ((predicted_time_11_pt <= predicted_time_11_kset)) {
            _Total_predict_time = timer.elapsed_ns();
            Total_predict_time += _Total_predict_time;
            timer.timeReset();
            tree.search(PT_packets[i]);
            _Total_search_time = timer.elapsed_ns();
            Total_search_time += _Total_search_time;
            ++model_counter_PT;
          } else {
            _Total_predict_time = timer.elapsed_ns();
            Total_predict_time += _Total_predict_time;
            timer.timeReset();
            if (num_set[0] > 0)
              kset_match_pri = set0.ClassifyAPacket(packets[i]);
            if (kset_match_pri < max_pri_set[1] && num_set[1] > 0)
              kset_match_pri =
                  max(kset_match_pri, set1.ClassifyAPacket(packets[i]));
            if (kset_match_pri < max_pri_set[2] && num_set[2] > 0)
              kset_match_pri =
                  max(kset_match_pri, set2.ClassifyAPacket(packets[i]));
            if (kset_match_pri < max_pri_set[3] && num_set[3] > 0)
              kset_match_pri =
                  max(kset_match_pri, set3.ClassifyAPacket(packets[i]));
            _Total_search_time = timer.elapsed_ns();
            Total_search_time += _Total_search_time;
            ++model_counter_KSet;
          }
#ifdef PERLOOKUPTIME_MODEL
          Total_y(i) = (_Total_search_time + _Total_predict_time);
#endif
        }
      }
      cout << "|=== AVG predict time(Model-11): "
           << Total_predict_time / (packetNum * trials) << "ns\n";
      cout << "|=== AVG search with predict time(Model-11): "
           << (Total_predict_time + Total_search_time) / (packetNum * trials)
           << "ns\n";
      cout << "|=== PT, DBT, KSET: "
           << (model_counter_PT / trials) / (packetNum * 1.0) << ", "
           << (model_counter_DBT / trials) / (packetNum * 1.0) << ", "
           << (model_counter_KSet / trials) / (packetNum * 1.0) << "\n";
#ifdef PERLOOKUPTIME_MODEL
      for (int i = 0; i < packetNum; ++i) {
        fprintf(total_model_11_fp, "Packet %d \t Time(ns) %f\n", i,
                Total_y(i) / 1.0);
      }
      fclose(total_model_11_fp);
#endif
    }
    ///////// Model Total /////////
    /*************************************************************************/

    /*************************************************************************/
    ///////// BloomFilter Construct /////////

#ifdef BLOOM
    {
      Eigen::VectorXd PT_y(packetNum);
      Eigen::VectorXd DBT_y(packetNum);
      Eigen::VectorXd KSet_y(packetNum);
      cout << ("\n**************** Classification(BLOOM) ****************\n");
      BloomFilter<(1 << 10), 4> bloom_filter_pt /*("./INFO/PT")*/;
      BloomFilter<(1 << 10), 4> bloom_filter_dbt /*("./INFO/DBT")*/;
      // BloomFilter<(1 << 12), 2> bloom_filter_kset;

      for (size_t t = 0; t < 2; ++t) {
        for (int i = 0; i < packetNum; ++i) {
#ifdef CACHE
          tree.search(PT_packets[i]);
#endif
          timer.timeReset();
          PT_match_id = tree.search(PT_packets[i]);
          _PT_search_time = timer.elapsed_ns();
          PT_y(i) = static_cast<double>(_PT_search_time);
        }
      }
      for (size_t t = 0; t < 2; ++t) {
        for (int i = 0; i < packetNum; ++i) {
#ifdef CACHE
          dbt.search(DBT_packets[i]);
#endif
          timer.timeReset();
          dbt.search(DBT_packets[i]);
          _DBT_search_time = timer.elapsed_ns();
          DBT_y(i) = static_cast<double>(_DBT_search_time);
        }
      }
      if (max_pri_set[1] < max_pri_set[2]) max_pri_set[1] = max_pri_set[2];
      for (size_t t = 0; t < 2; ++t) {
        for (size_t i = 0; i < packetNum; ++i) {
#ifdef CACHE
          kset_match_pri = -1;
          if (num_set[0] > 0) kset_match_pri = set0.ClassifyAPacket(packets[i]);
          if (kset_match_pri < max_pri_set[1] && num_set[1] > 0)
            kset_match_pri =
                max(kset_match_pri, set1.ClassifyAPacket(packets[i]));
          if (kset_match_pri < max_pri_set[2] && num_set[2] > 0)
            kset_match_pri =
                max(kset_match_pri, set2.ClassifyAPacket(packets[i]));
          if (kset_match_pri < max_pri_set[3] && num_set[3] > 0)
            kset_match_pri =
                max(kset_match_pri, set3.ClassifyAPacket(packets[i]));
#endif
          kset_match_pri = -1;
          timer.timeReset();
          if (num_set[0] > 0) kset_match_pri = set0.ClassifyAPacket(packets[i]);
          if (kset_match_pri < max_pri_set[1] && num_set[1] > 0)
            kset_match_pri =
                max(kset_match_pri, set1.ClassifyAPacket(packets[i]));
          if (kset_match_pri < max_pri_set[2] && num_set[2] > 0)
            kset_match_pri =
                max(kset_match_pri, set2.ClassifyAPacket(packets[i]));
          if (kset_match_pri < max_pri_set[3] && num_set[3] > 0)
            kset_match_pri =
                max(kset_match_pri, set3.ClassifyAPacket(packets[i]));
          _KSet_search_time = timer.elapsed_ns();
          KSet_y(i) = static_cast<double>(_KSet_search_time);
        }
      }

      for (size_t i = 0; i < packetNum; ++i) {
        if (PT_y(i) >= per75_PT) {
          bloom_filter_pt.insert(PT_packets[i]);
        }
        if (DBT_y(i) >= per75_DBT) {
          bloom_filter_dbt.insert(DBT_packets[i]);
        }
      }
///////// BloomFilter Construct /////////
/*************************************************************************/

/*************************************************************************/
///////// BloomFilter Classification /////////
#ifdef PERLOOKUPTIME_BLOOM
      FILE *Bloom_res = nullptr;
      Bloom_res = fopen("./INFO/BloomResults.txt", "w");
      FILE *Bloom_csv_fp = nullptr;
      Bloom_csv_fp = fopen("./INFO/Bloom_Results.csv", "w");
      uint32_t bloom_match_id_arr[packetNum];
      unsigned long long perLook_bloom[packetNum * trials][2];
#endif

      Total_predict_time = 0;
      PT_search_time = 0;
      DBT_search_time = 0;
      KSet_search_time = 0;
      int model_counter_DBT = 0;
      int model_counter_PT = 0;
      int model_counter_KSet = 0;

      warmup_KSet(set, packets, packetNum, num_set);
      warmup_PT(tree, PT_packets, packetNum);
      warmup_DBT(dbt, DBT_packets, packetNum);
      if (max_pri_set[1] < max_pri_set[2]) max_pri_set[1] = max_pri_set[2];
      for (size_t t = 0; t < trials; ++t) {
        for (int i = 0; i < packetNum; ++i) {
          timer.timeReset();
          if (!bloom_filter_dbt.contains(DBT_packets[i])) {
            _Total_predict_time = timer.elapsed_ns();
            Total_predict_time += _Total_predict_time;
            timer.timeReset();
#ifndef PERLOOKUPTIME_BLOOM
            dbt.search(DBT_packets[i]);
            _DBT_search_time = timer.elapsed_ns();
            DBT_search_time += _DBT_search_time;
#else
            DBT_match_id = dbt.search(DBT_packets[i]);
            _DBT_search_time = timer.elapsed_ns();
            DBT_search_time += _DBT_search_time;
            --DBT_match_id;
            bloom_match_id_arr[i] = DBT_match_id;
#endif
#ifdef PERLOOKUPTIME_BLOOM
            perLook_bloom[t * packetNum + i][0] = _DBT_search_time;
            perLook_bloom[t * packetNum + i][1] = 1;
#endif
            ++model_counter_DBT;
          } else if (!bloom_filter_pt.contains(PT_packets[i])) {
            _Total_predict_time = timer.elapsed_ns();
            Total_predict_time += _Total_predict_time;
            timer.timeReset();
#ifndef PERLOOKUPTIME_BLOOM
            tree.search(PT_packets[i]);
            _PT_search_time = timer.elapsed_ns();
            PT_search_time += _PT_search_time;
#else
            PT_match_id = tree.search(PT_packets[i]);
            _PT_search_time = timer.elapsed_ns();
            PT_search_time += _PT_search_time;
            --PT_match_id;
            bloom_match_id_arr[i] = static_cast<uint32_t>(PT_match_id);
#endif
#ifdef PERLOOKUPTIME_BLOOM
            perLook_bloom[t * packetNum + i][0] = _PT_search_time;
            perLook_bloom[t * packetNum + i][1] = 0;
#endif
            ++model_counter_PT;
          } else {
            _Total_predict_time = timer.elapsed_ns();
            Total_predict_time += _Total_predict_time;
            kset_match_pri = -1;
            timer.timeReset();
            if (num_set[0] > 0)
              kset_match_pri = set0.ClassifyAPacket(packets[i]);
            if (kset_match_pri < max_pri_set[1] && num_set[1] > 0)
              kset_match_pri =
                  max(kset_match_pri, set1.ClassifyAPacket(packets[i]));
            if (kset_match_pri < max_pri_set[2] && num_set[2] > 0)
              kset_match_pri =
                  max(kset_match_pri, set2.ClassifyAPacket(packets[i]));
            if (kset_match_pri < max_pri_set[3] && num_set[3] > 0)
              kset_match_pri =
                  max(kset_match_pri, set3.ClassifyAPacket(packets[i]));
            _KSet_search_time = timer.elapsed_ns();
            KSet_search_time += _KSet_search_time;
#ifdef PERLOOKUPTIME_BLOOM
            bloom_match_id_arr[i] =
                static_cast<uint32_t>((number_rule - 1) - kset_match_pri);
#endif
#ifdef PERLOOKUPTIME_BLOOM
            perLook_bloom[t * packetNum + i][0] = _KSet_search_time;
            perLook_bloom[t * packetNum + i][1] = 2;
#endif
            ++model_counter_KSet;
          }
        }
      }
      cout << "|=== AVG predict time(BloomFilter): "
           << Total_predict_time / (packetNum * trials) << "ns\n";
      cout << "|=== AVG search time with predict(BloomFilter): "
           << (Total_predict_time + PT_search_time + DBT_search_time +
               KSet_search_time) /
                  (packetNum * trials)
           << "ns\n";
      cout << "|=== PT, DBT, KSET: "
           << (model_counter_PT / trials) / (packetNum * 1.0) << ", "
           << (model_counter_DBT / trials) / (packetNum * 1.0) << ", "
           << (model_counter_KSet / trials) / (packetNum * 1.0) << "\n";

#ifdef PERLOOKUPTIME_BLOOM
      // 寫入 CSV 表頭
      fprintf(Bloom_csv_fp, "LookupTime(ns),MethodIndex\n");

      // 將每筆資料寫入檔案
      for (size_t i = 0; i < packetNum * trials; ++i) {
        fprintf(Bloom_csv_fp, "%llu,%llu\n", perLook_bloom[i][0],
                perLook_bloom[i][1]);
      }
      fclose(Bloom_csv_fp);
      for (int i = 0; i < packetNum; ++i) {
        fprintf(Bloom_res, "Packet %d \t Result %u \t Time(ns) %f\n", i,
                bloom_match_id_arr[i], perLook_bloom[packetNum + i][0] / 1.0);
      }
      fclose(Bloom_res);
      // 輸出 bit array 狀態
      bloom_filter_pt.dump_bit_array("./INFO/bloom_filter_pt.txt");
      bloom_filter_dbt.dump_bit_array("./INFO/bloom_filter_dbt.txt");
      // bloom_filter_kset.dump_bit_array("./INFO/bloom_filter_kset.txt");
#endif

      ///////// BloomFilter Classification /////////
      /*************************************************************************/
    }
#endif
    /*************************************************************************/
    ///////// Individual /////////
    {
      Eigen::VectorXd PT_y(packetNum);
      Eigen::VectorXd DBT_y(packetNum);
      Eigen::VectorXd KSet_y(packetNum);
      int PT_match_id_arr[packetNum];
      uint32_t DBT_match_id_arr[packetNum];
      int KSet_match_pri_arr[packetNum];
      PT_search_time = 0;
      DBT_search_time = 0;
      KSet_search_time = 0;
      cout << ("\n************** Classification(Individual) **************");
      // PT classification
      cout << ("\n**************** Classification(PT) ****************\n");
#ifdef PERLOOKUPTIME_INDIVIDUAL
      FILE *PT_res_fp = nullptr;
      PT_res_fp = fopen("./INFO/PT_IndivResults.txt", "w");
#endif

      // warmup_PT(tree, PT_packets, packetNum);
      for (size_t t = 0; t < trials; ++t) {
        for (int i = 0; i < packetNum; ++i) {
          timer.timeReset();
          (PT_match_id = tree.search(PT_packets[i]))--;
          _PT_search_time = timer.elapsed_ns();
          PT_search_time += _PT_search_time;

#ifdef PERLOOKUPTIME_INDIVIDUAL
          PT_y(i) = static_cast<double>(_PT_search_time);
          PT_match_id_arr[i] = PT_match_id;
#endif
        }
      }

#ifdef PERLOOKUPTIME_INDIVIDUAL
      for (int i = 0; i < packetNum; ++i) {
        fprintf(PT_res_fp, "Packet %d \t Result %d \t Time(ns) %f\n", i,
                (PT_match_id_arr[i]), PT_y(i) / 1.0);
      }
      fclose(PT_res_fp);
#endif

      cout << "|- Average search time: "
           << PT_search_time / (packetNum * trials) << "ns\n";
      //// PT ////

      // DBT classification
      cout << ("\n**************** Classification(DBT) ****************\n");
#ifdef PERLOOKUPTIME_INDIVIDUAL
      FILE *DBT_res_fp = nullptr;
      DBT_res_fp = fopen("./INFO/DBT_IndivResults.txt", "w");
#endif

      // warmup_DBT(dbt, DBT_packets, packetNum);
      for (size_t t = 0; t < trials; ++t) {
        for (int i = 0; i < packetNum; ++i) {
          timer.timeReset();
          (DBT_match_id = dbt.search(DBT_packets[i]))--;
          _DBT_search_time = timer.elapsed_ns();
          DBT_search_time += _DBT_search_time;

#ifdef PERLOOKUPTIME_INDIVIDUAL
          DBT_y(i) = static_cast<double>(_DBT_search_time);
          DBT_match_id_arr[i] = DBT_match_id;
#endif
        }
      }

#ifdef PERLOOKUPTIME_INDIVIDUAL
      for (int i = 0; i < packetNum; ++i) {
        fprintf(DBT_res_fp, "Packet %d \t Result %u \t Time(ns) %f\n", i,
                DBT_match_id_arr[i], DBT_y(i) / 1.0);
      }
      fclose(DBT_res_fp);
#endif

      cout << "|- Average search time: "
           << DBT_search_time / (packetNum * trials) << "ns\n";
      dbt.search_with_log(DBT_packets);
      //// DBT ////

      //// KSet ////
      // KSet classification
      cout << ("\n**************** Classification(KSet) ****************\n");
      // JIA
      // if (max_pri_set[1] < max_pri_set[2]) max_pri_set[1] = max_pri_set[2];

#ifdef PERLOOKUPTIME_INDIVIDUAL
      FILE *KSet_res_fp = nullptr;
      KSet_res_fp = fopen("./INFO/KSet_IndivResults.txt", "w");
#endif

      // warmup_KSet(set,packets,packetNum,num_set);
      if (max_pri_set[1] < max_pri_set[2]) max_pri_set[1] = max_pri_set[2];
      for (size_t t = 0; t < trials; ++t) {
        for (size_t i = 0; i < packetNum; ++i) {
          timer.timeReset();
          kset_match_pri = -1;
          if (num_set[0] > 0) kset_match_pri = set0.ClassifyAPacket(packets[i]);
          if (kset_match_pri < max_pri_set[1] && num_set[1] > 0)
            kset_match_pri =
                max(kset_match_pri, set1.ClassifyAPacket(packets[i]));
          if (kset_match_pri < max_pri_set[2] && num_set[2] > 0)
            kset_match_pri =
                max(kset_match_pri, set2.ClassifyAPacket(packets[i]));
          if (kset_match_pri < max_pri_set[3] && num_set[3] > 0)
            kset_match_pri =
                max(kset_match_pri, set3.ClassifyAPacket(packets[i]));
          _KSet_search_time = timer.elapsed_ns();
          KSet_search_time += _KSet_search_time;

#ifdef PERLOOKUPTIME_INDIVIDUAL
          KSet_y(i) = static_cast<double>(_KSet_search_time);
          KSet_match_pri_arr[i] = (number_rule - 1) - kset_match_pri;
#endif
        }
      }

#ifdef PERLOOKUPTIME_INDIVIDUAL
      for (int i = 0; i < packetNum; ++i) {
        fprintf(KSet_res_fp, "Packet %d \t Result %u \t Time(ns) %f\n", i,
                KSet_match_pri_arr[i], KSet_y(i) / 1.0);
      }
      fclose(KSet_res_fp);
#endif

      cout << fixed << setprecision(3)  // 設定小數點後 3 位
           << "\tTotal classification time: " << (KSet_search_time * trials)
           << " ns" << endl
           << "\tAverage classification time: "
           << (KSet_search_time / (trials * packetNum)) << " ns\n";
    }
    ///////// Individual /////////
    /*************************************************************************/
  }

  return 0;
}
