// #include <stdio.h>  // JIA remove printf
// #include <stdlib.h>
// #include <unistd.h>

#include "LinearRegressionModel.hpp"

//// KSet ////
#include "KSet.hpp"
#include "basis.hpp"
#include "input.hpp"
// #include "inputFile_test.hpp"
// #include "linearSearch_test.hpp"
//// KSet ////

//// DT MT ////
#include "dynamictuple.h"
// #include "elementary_DT_MT.h"
#include "multilayertuple.h"
//// DT MT ////

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

// #define VALID
// #define SAMPLE
#define TIMER_METHOD TIMER_RDTSCP
// #define TIMER_METHOD TIMER_STEADY_CLOCK
#define THREAD_NUM
#define BIAS
#define NORM
#define CACHE
#define EIGEN_NO_DEBUG  // 關閉 Eigen assert
// #define EIGEN_UNROLL_LOOP_LIMIT 64
#define PERLOOKUPTIME_MODEL
#define PERLOOKUPTIME_KNN
///////// Shuffle /////////
// #define SHUFFLE
#ifdef SHUFFLE
#include <random>
#endif
///////// Shuffle /////////
///////// MP /////////
#include <omp.h>
///////// MP /////////
///////// KNN /////////
#define KNN_MODEL
#ifdef KNN_MODEL
#include "KNNClassifier.hpp"
#endif
///////// KNN /////////
///////// bloomFilter /////////
#define BLOOM
#ifdef BLOOM
#include "bloomFilter.hpp"
#endif
///////// bloomFilter /////////

// constexpr const char *LoadRule_test_path = "./INFO/loadRule_test.txt";
// constexpr const char *LoadPacket_test_path = "./INFO/loadPacket_test.txt";
// 靜態成員初始化
struct option CommandLineParser::long_options[] = {
    {"ruleset", required_argument, NULL, 'r'},
    {"trace", required_argument, NULL, 'p'},
    {"test", no_argument, NULL, 't'},
    {"classification", required_argument, NULL, 's'},
    {"update", no_argument, NULL, 'u'},
    {"fields_PT", no_argument, NULL, 'f'},
    {"Top-K_DBT", no_argument, NULL, 'k'},
    {"C_BOUND_DBT", no_argument, NULL, 'c'},
    {"END_BOUND_DBT", no_argument, NULL, 'e'},
    {"BINTH_DBT", no_argument, NULL, 'b'},
    {"pfx_dim_DT_MT", no_argument, NULL, 'd'},
    {"help", no_argument, NULL, 'h'},
    {0, 0, 0, 0}  // 結束標記
};

// JIA  int max_pri_set[4]
void anaK(const size_t number_rule, const vector<Rule_KSet> &rule,
          int *usedbits, vector<Rule_KSet> set[4], int k, int max_pri_set[4]) {
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
  cout << ("\n**************** Construction(KSet) ****************\n");
  cout << ("================KSet Precompute=============\n");
  cout << "Set 0: " << pre_seg[0] << ", Set 1: " << pre_seg[1]
       << ", Set 2: " << pre_seg[2] << ", Set 3: " << pre_seg[3] << "\n";

  cout << "max_pri[0]: " << max_pri_set[0] << ", max_pri[1]: " << max_pri_set[1]
       << ", max_pri[2]: " << max_pri_set[2]
       << ", max_pri[3]: " << max_pri_set[3] << "\n";
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
       << "Set 2: " << usedbits[2] << " bits" << "\n";
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
        ++Set[0].non_empty_seg;
      }
      ++Set[0].seg[hash];
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
       << ", Set 2: " << set[2].size() << ", Set 3: " << set[3].size() << "\n";

  cout << "max_pri[0]: " << max_pri_set[0] << ", max_pri[1]: " << max_pri_set[1]
       << ", max_pri[2]: " << max_pri_set[2]
       << ", max_pri[3]: " << max_pri_set[3] << "\n";

  cout << "non-empty_seg[0] = " << Set[0].non_empty_seg
       << ", non-empty_seg[1] = " << Set[1].non_empty_seg
       << ", non-empty_seg[2] = " << Set[2].non_empty_seg << "\n";

  cout << fixed << setprecision(3)
       << "AVG[0]: " << (float)set[0].size() / Set[0].non_empty_seg
       << ", AVG[1]: " << (float)set[1].size() / Set[1].non_empty_seg
       << ", AVG[2]: " << (float)set[2].size() / Set[2].non_empty_seg << "\n";

  cout << "MAX[0]: " << max[0] << ", MAX[1]: " << max[1]
       << ", MAX[2]: " << max[2] << "\n";
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
// Convert Rule_KSet -> PT_Rule
// ===============================
vector<PT_Rule> convertToPTRules(const vector<Rule_KSet> &rules) {
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
vector<Rule_KSet> convertFromPTRules(const vector<PT_Rule> &pt_rules) {
  vector<Rule_KSet> rules;

  for (const auto &pt : pt_rules) {
    Rule_KSet r{};
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

vector<DBT::Rule> convertToDBTRules(const vector<Rule_KSet> &rules) {
  vector<DBT::Rule> dbt_rules;

  for (const Rule_KSet &r : rules) {
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
vector<Rule_KSet> convertFromDBTRules(const vector<DBT::Rule> &dbt_rules) {
  vector<Rule_KSet> rules;

  for (const auto &dbt_r : dbt_rules) {
    Rule_KSet r{};
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

vector<Trace *> convertPackets_KSet2DTMT(const vector<Packet> &packets) {
  vector<Trace *> traces;
  for (const auto &packet : packets) {
    Trace *trace = (Trace *)malloc(sizeof(Trace));
    // Copy key data for up to 5 dimensions
    for (size_t i = 0; i < 5; ++i) {
      trace->key[i] = packet[i];
    }
    traces.emplace_back(trace);
  }
  traces.shrink_to_fit();
  return traces;
}

vector<Packet> convertPackets_DTMT2KSet(const vector<Trace *> &traces) {
  vector<Packet> packets;
  for (const auto &trace : traces) {
    Packet packet;
    // Copy key data for up to 5 dimensions
    for (size_t i = 0; i < 5; ++i) {
      packet[i] = trace->key[i];
    }
    // If Packet has more dimensions than 5, initialize the rest to 0
    for (size_t i = 5; i < packet.size(); ++i) {
      packet[i] = 0;
    }
    packets.emplace_back(packet);
  }
  packets.shrink_to_fit();
  return packets;
}

vector<Rule_DT_MT *> convertRules_KSetToDTMT(
    const vector<Rule_KSet> &ksetRules) {
  vector<Rule_DT_MT *> dtmtRules;
  for (const auto &ksetRule : ksetRules) {
    Rule_DT_MT *dtmtRule = (Rule_DT_MT *)malloc(sizeof(Rule_DT_MT));
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
    const vector<Rule_DT_MT *> &dtmtRules) {
  vector<Rule_KSet> ksetRules;
  for (const auto &dtmtRule : dtmtRules) {
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

/* JIA */ __attribute__((always_inline)) inline void warmup_PT(
    PT::PTtree &tree, const vector<PT::PT_Packet> &PT_packets) {
  for (size_t i = 0; i < 2; ++i) {
    tree.search(PT_packets[i]);
  }
}
/* JIA */ __attribute__((always_inline)) inline void warmup_DBT(
    DBT::DBTable &dbt, const vector<DBT::Packet> &DBT_packets) {
  for (size_t i = 0; i < 2; ++i) {
    dbt.search(DBT_packets[i]);
  }
}
/* JIA */ __attribute__((always_inline)) inline void warmup_KSet(
    vector<KSet> &set, const vector<Packet> &packets, const int num_set[],
    const int max_pri_set[4]) {
  for (size_t i = 0; i < 2; ++i) {
    int kset_match_pri = -1;
    if (num_set[0] > 0) kset_match_pri = set[0].ClassifyAPacket(packets[i]);
    if (kset_match_pri < max_pri_set[1] && num_set[1] > 0)
      kset_match_pri = max(kset_match_pri, set[1].ClassifyAPacket(packets[i]));
    if (__builtin_expect(kset_match_pri < max_pri_set[2] && num_set[2] > 0, 1))
      kset_match_pri = max(kset_match_pri, set[2].ClassifyAPacket(packets[i]));
    if (__builtin_expect(kset_match_pri < max_pri_set[3] && num_set[3] > 0, 0))
      kset_match_pri = max(kset_match_pri, set[3].ClassifyAPacket(packets[i]));
  }
}
/* JIA */ __attribute__((always_inline)) inline void warmup_DT(
    DynamicTuple &dynamictuple, const vector<Trace *> &traces_DT_MT) {
  for (size_t i = 0; i < 2; ++i) {
    (dynamictuple.Lookup(traces_DT_MT[i], 0));
  }
}
/* JIA */ __attribute__((always_inline)) inline void warmup_MT(
    MultilayerTuple &multilayertuple, const vector<Trace *> &traces_DT_MT) {
  for (size_t i = 0; i < 2; ++i) {
    (multilayertuple.Lookup(traces_DT_MT[i], 0));
  }
}
void convert_packets_to_datas(const vector<Packet> &packets,
                              vector<array<double, 5>> &datas) {
  datas.clear();
  datas.reserve(packets.size());  // 預先分配記憶體以避免 realloc

  for (const auto &pkt : packets) {
    array<double, 5> data;
    for (size_t i = 0; i < 5; ++i) {
      data[i] = static_cast<double>(pkt[i]);  // 保留轉型語意清晰
    }
    datas.emplace_back(data);
  }
  datas.shrink_to_fit();
}
#ifdef SAMPLE
// 每 N 組資料中，有 K 組進 sample，其餘進 test（留在原 data）
// data: 原始資料，在函式結束後只保留 test 資料；
// sample: 存放取出的樣本。
// group_size: 每幾筆資料為一組（例如每 5 筆為一組）；
// sample_count: 每組中前幾筆進入 sample；
void split_sample_test(vector<Packet> &data, vector<Packet> &sample,
                       size_t group_size, size_t sample_count) {
  sample.clear();
  if (group_size == 0 || sample_count > group_size) {
    throw std::invalid_argument("Invalid group_size or sample_count");
  }

  size_t write_idx = 0;
  const size_t total = data.size();

  for (size_t i = 0; i < total; i += group_size) {
    // 拿前 sample_count 筆進 sample（若不足也盡量拿）
    for (size_t j = 0; j < sample_count && i + j < total; ++j) {
      sample.push_back(std::move(data[i + j]));
    }

    // 其餘進 test（覆蓋 data）
    for (size_t j = sample_count; j < group_size && i + j < total; ++j) {
      data[write_idx++] = std::move(data[i + j]);
    }
  }

  data.resize(write_idx);  // 移除已移出的 sample 空間
  data.shrink_to_fit();
  sample.shrink_to_fit();
}
#endif
void normalize_first_two_dims(vector<Packet> &data) {
  if (data.empty()) return;

  uint32_t min0 = UINT32_MAX, max0 = 0;
  uint32_t min1 = UINT32_MAX, max1 = 0;

  // 先找出第一、二維的最小與最大值
  for (const auto &item : data) {
    min0 = std::min(min0, item[0]);
    max0 = std::max(max0, item[0]);

    min1 = std::min(min1, item[1]);
    max1 = std::max(max1, item[1]);
  }

  // 避免除以 0
  double range0 = (max0 > min0) ? static_cast<double>(max0 - min0) : 1.0;
  double range1 = (max1 > min1) ? static_cast<double>(max1 - min1) : 1.0;

  // 對每筆資料的前兩維做 normalization（轉為 double 儲存）
  for (auto &item : data) {
    double norm0 = static_cast<double>(item[0] - min0) / range0;
    double norm1 = static_cast<double>(item[1] - min1) / range1;

    // 若要保留回原位（但 uint32_t 無法表示小數）→ 需轉存 double 結構或另建結構
    // 範例：乘回來後儲存
    item[0] = static_cast<uint32_t>(norm0 * 1e10);  // scale to [0, 1e10]
    item[1] = static_cast<uint32_t>(norm1 * 1e10);
  }
}
/////////////////
int main(int argc, char *argv[]) {
  ios::sync_with_stdio(false);
  cin.tie(nullptr);

  CommandLineParser parser;
  parser.parseArguments(argc, argv);
  float ip_bytes[4] = {0};
  vector<Rule_KSet> rule;
  vector<Packet> packets;
  InputFile inputFile;
  // InputFile_test inputFile_test;
  Timer timer;
  constexpr int trials = 4;  // run 4 times circularly

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
#ifdef SAMPLE
  vector<Packet> samples;
  // // 每 2 筆資料中，前 1 筆進 samples，剩下 1 筆留在 packets
  // split_sample_test(packets, samples, 2, 1);
  // 每 5 筆資料中，前 4 筆進 samples，剩下 1 筆留在 packets
  // split_sample_test(packets, samples, 5, 4);
  // // 每 10 筆資料中，前 1 筆進 samples，剩下 9 筆留在 packets
  // split_sample_test(packets, samples, 10, 1);
  // // 每 20 筆資料中，前 1 筆進 samples，剩下 19 筆留在 packets
  split_sample_test(packets, samples, 20, 1);
  const size_t packetNum = packets.size();
  const size_t sampleNum = samples.size();
#else
  // 若未定義 SAMPLE，samples 即為原始 packets（不拆分）
  vector<Packet> &samples = packets;
  const size_t packetNum = packets.size();
  const size_t &sampleNum = packetNum;
#endif
  cout << "The number of packets = " << packetNum << "\n";
  cout << "The number of samples = " << sampleNum << "\n";

#ifdef SHUFFLE
  // 初始化亂數生成器
  random_device rd;
  mt19937 gen(rd());
  // 使用 shuffle 打亂整個 array 為單位的順序
  shuffle(packets.begin(), packets.end(), gen);
#endif
#if TIMER_METHOD == TIMER_RDTSCP
  // 手動預熱，避免第一次調用 elapsed_ns() 時的額外延遲
  Timer::warmup();
#endif
  /*************************************************************************/
  ///////// DS Construct /////////
  //// PT ////
  auto PT_packets = convertToPTPackets(packets);
  auto PT_rules = convertToPTRules(rule);
#ifdef SAMPLE
  auto PT_samples = convertToPTPackets(samples);
#else
  auto &PT_samples = PT_packets;
#endif

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

    cout << "\nPT search config time: " << timer.elapsed_s() << " s" << "\n";
  }
  for (size_t i = 0; i < set_field.size(); ++i)
    cout << static_cast<int>(set_field[i]) << " ";
  PTtree tree(set_field, set_port);

  // insert
  // PT construct
  cout << ("\n**************** Construction(PT) ****************\n");
  cout << "\nStart build for single thread...\n|- Using fields:     ";
  for (unsigned int x : set_field) cout << x << ",";
  cout << set_port << "\n";
  timer.timeReset();
  for (auto &&r : PT_rules) {
    tree.insert(r);
  }
  cout << "|- Construct time:   " << timer.elapsed_ns() << "ns\n";
  cout << "|- tree.totalNodes: " << tree.totalNodes << "\n";

  cout << "|- Memory footprint: " << (double)tree.mem() / 1024.0 / 1024.0
       << "MB\n";
  //// PT ////

  //// DBT ////
  auto DBT_packets = convertToDBTPackets(packets);
  auto DBT_rules = convertToDBTRules(rule);
#ifdef SAMPLE
  auto DBT_samples = convertToDBTPackets(samples);
#else
  auto &DBT_samples = DBT_packets;
#endif
  cout << "binth=" << DBT::BINTH << " th_b=" << DBT::END_BOUND
       << " K=" << DBT::TOP_K << " th_c=" << DBT::C_BOUND << "\n"
       << "\n";
  DBT::DBTable dbt(DBT_rules, DBT::BINTH);
  // DBT construct
  cout << ("**************** Construction(DBT) ****************\n");
  timer.timeReset();
  dbt.construct();
  cout << "Construction Time: " << timer.elapsed_ns() << " ns\n";
  dbt.mem();
  //// DBT ////

  //// KSet ////
  // JIA for KSet
  constexpr int pre_K = 16;
  int SetBits[3] = {8, 8, 8};
  int max_pri_set[4] = {-1, -1, -1, -1};
  int kset_match_pri = -1;

  vector<Rule_KSet> set_4[4];
  anaK(number_rule, rule, SetBits, set_4, pre_K, max_pri_set);

  int num_set[4] = {0};
  for (size_t i = 0; i < 4; ++i) {
    num_set[i] = set_4[i].size();
  }

  // JIA
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
  // JIA
  KSet &set0 = set[0];
  KSet &set1 = set[1];
  KSet &set2 = set[2];
  KSet &set3 = set[3];

  // KSet construct
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
  set.shrink_to_fit();  // JIA
  set0.prints();
  set1.prints();
  set2.prints();
  set3.prints();
  //// KSet ////

  //// DT MT ////
  auto traces_DT_MT = convertPackets_KSet2DTMT(packets);
  auto rules_DT_MT = convertRules_KSetToDTMT(rule);
#ifdef SAMPLE
  auto DT_MT_samples = convertPackets_KSet2DTMT(samples);
#else
  auto &DT_MT_samples = traces_DT_MT;
#endif
  // if (command.prefix_dims_num == 5) rules_DT_MT =
  // RulesPortPrefix(rules_DT_MT, true);
  cout << ("**************** Construction(DT) ****************\n");
  DynamicTuple dynamictuple;
  dynamictuple.use_port_hash_table = 1;
  dynamictuple.threshold = 7;
  timer.timeReset();
  dynamictuple.Create(rules_DT_MT, true);
  cout << "\tConstruction time: " << timer.elapsed_ns() << " ns\n";
  // memory
  auto DT_data_memory_size =
      sizeof(Rule_DT_MT) * rules_DT_MT.size() / 1024.0 / 1024.0;
  auto DT_index_memory_size = dynamictuple.MemorySize() / 1024.0 / 1024.0;
  cout << "DT_data_memory_size: " << DT_data_memory_size
       << ", DT_index_memory_size: " << DT_index_memory_size << "\n"
       << "Total(MB): " << DT_data_memory_size + DT_index_memory_size << "\n";
  cout << "DT tuples_num: " << dynamictuple.tuples_num << "\n";

  cout << ("**************** Construction(MT) ****************\n");
  MultilayerTuple multilayertuple;

  //  if (command.next_layer_rules_num > 0) {
  //     create_next_layer_rules_num = command.next_layer_rules_num;
  //     delete_next_layer_rules_num = command.next_layer_rules_num / 2;
  //   }

  multilayertuple.Init(1, true);
  timer.timeReset();
  multilayertuple.Create(rules_DT_MT, true);
  cout << "\tConstruction time: " << timer.elapsed_ns() << " ns\n";
  // memory
  auto MT_data_memory_size =
      sizeof(Rule_DT_MT) * rules_DT_MT.size() / 1024.0 / 1024.0;
  auto MT_index_memory_size = multilayertuple.MemorySize() / 1024.0 / 1024.0;
  cout << "MT_data_memory_size: " << MT_data_memory_size
       << ", MT_index_memory_size: " << MT_index_memory_size << "\n"
       << "Total(MB): " << MT_data_memory_size + MT_index_memory_size << "\n";
  cout << "MT tuples_num: " << multilayertuple.tuples_num << "\n";
  //// DT MT ////
  double PT_search_time = 0, _PT_search_time = 0;
  double DBT_search_time = 0, _DBT_search_time = 0;
  double KSet_search_time = 0, _KSet_search_time = 0;
  double DT_search_time = 0, _DT_search_time = 0;
  double MT_search_time = 0, _MT_search_time = 0;
  int PT_match_id = 0;
  uint32_t DBT_match_id = 0;
  vector<int> predict_choose(packetNum);
  ///////// DS Construct /////////
  /*************************************************************************/

  /*************************************************************************/
  ///////// Baseline /////////
  // y 向量共用 (3, 5, 11)
  Eigen::VectorXd PT_y(sampleNum);
  Eigen::VectorXd DBT_y(sampleNum);
  Eigen::VectorXd KSet_y(sampleNum);
  Eigen::VectorXd DT_y(sampleNum);
  Eigen::VectorXd MT_y(sampleNum);
#if defined(PERLOOKUPTIME_MODEL) || defined(PERLOOKUPTIME_KNN)
  Eigen::VectorXd Total_y(packetNum);
#endif
#if TIMER_METHOD == TIMER_RDTSCP
  Timer::warmup();
#endif
  for (int t = 0; t < 2; ++t) {
    for (size_t i = 0; i < sampleNum; ++i) {
// 搜尋時間量測 (PT)
#ifdef CACHE
      tree.search(PT_samples[i]);
#endif
      timer.timeReset();
      tree.search(PT_samples[i]);
      _PT_search_time = timer.elapsed_ns();
      // PT_y(i) = static_cast<double>(_PT_search_time);
      PT_y(i) += static_cast<double>(_PT_search_time);
    }
  }
  for (int t = 0; t < 2; ++t) {
    for (size_t i = 0; i < sampleNum; ++i) {
// 搜尋時間量測 (DBT)
#ifdef CACHE
      dbt.search(DBT_samples[i]);
#endif
      timer.timeReset();
      dbt.search(DBT_samples[i]);
      _DBT_search_time = timer.elapsed_ns();
      // DBT_y(i) = static_cast<double>(_DBT_search_time);
      DBT_y(i) += static_cast<double>(_DBT_search_time);
    }
  }
  if (max_pri_set[1] < max_pri_set[2]) max_pri_set[1] = max_pri_set[2];
  for (int t = 0; t < 2; ++t) {
    for (size_t i = 0; i < sampleNum; ++i) {
// 搜尋時間量測 (KSet)
#ifdef CACHE
      kset_match_pri = -1;
      if (__builtin_expect(num_set[0] > 0, 1))
        kset_match_pri = set0.ClassifyAPacket(samples[i]);
      if (__builtin_expect(kset_match_pri < max_pri_set[1] && num_set[1] > 0,
                           1))
        kset_match_pri = max(kset_match_pri, set1.ClassifyAPacket(samples[i]));
      if (__builtin_expect(kset_match_pri < max_pri_set[2] && num_set[2] > 0,
                           1))
        kset_match_pri = max(kset_match_pri, set2.ClassifyAPacket(samples[i]));
      if (__builtin_expect(kset_match_pri < max_pri_set[3] && num_set[3] > 0,
                           0))
#endif

        timer.timeReset();
      kset_match_pri = -1;
      if (__builtin_expect(num_set[0] > 0, 1))
        kset_match_pri = set0.ClassifyAPacket(samples[i]);
      if (__builtin_expect(kset_match_pri < max_pri_set[1] && num_set[1] > 0,
                           1))
        kset_match_pri = max(kset_match_pri, set1.ClassifyAPacket(samples[i]));
      if (__builtin_expect(kset_match_pri < max_pri_set[2] && num_set[2] > 0,
                           1))
        kset_match_pri = max(kset_match_pri, set2.ClassifyAPacket(samples[i]));
      if (__builtin_expect(kset_match_pri < max_pri_set[3] && num_set[3] > 0,
                           0))
        kset_match_pri = max(kset_match_pri, set3.ClassifyAPacket(samples[i]));
      _KSet_search_time = timer.elapsed_ns();
      // KSet_y(i) = static_cast<double>(_KSet_search_time);
      KSet_y(i) += static_cast<double>(_KSet_search_time);
    }
  }
  for (int t = 0; t < 2; ++t) {
    for (size_t i = 0; i < sampleNum; ++i) {
// 搜尋時間量測 (DT)
#ifdef CACHE
      (dynamictuple.Lookup(DT_MT_samples[i], 0));
#endif
      timer.timeReset();
      (dynamictuple.Lookup(DT_MT_samples[i], 0));
      _DT_search_time = timer.elapsed_ns();
      // DT_y(i) = static_cast<double>(_DT_search_time);
      DT_y(i) += static_cast<double>(_DT_search_time);
    }
  }
  for (int t = 0; t < 2; ++t) {
    for (size_t i = 0; i < sampleNum; ++i) {
// 搜尋時間量測 (MT)
#ifdef CACHE
      (multilayertuple.Lookup(DT_MT_samples[i], 0));
#endif
      timer.timeReset();
      (multilayertuple.Lookup(DT_MT_samples[i], 0));
      _MT_search_time = timer.elapsed_ns();
      // MT_y(i) = static_cast<double>(_MT_search_time);
      MT_y(i) += static_cast<double>(_MT_search_time);
    }
  }
  for (size_t i = 0; i < sampleNum; ++i) {
    MT_y(i) = (MT_y(i) / 2.0);
    DT_y(i) = (DT_y(i) / 2.0);
    KSet_y(i) = (KSet_y(i) / 2.0);
    DBT_y(i) = (DBT_y(i) / 2.0);
    PT_y(i) = (PT_y(i) / 2.0);
  }
  ///////// Baseline /////////
  /*************************************************************************/

  if (parser.isSearchMode()) {
    double Total_search_time = 0, _Total_search_time = 0;
    double Omp_predict_time = 0, Sig_predict_time = 0;
    /*************************************************************************/
    ///////// Linear Model /////////
    {
      cout << ("\n*********\n")
           << ("**************** Build(Linear Model) "
               "****************\n");
      cout << "\tThe number of test packet = " << packetNum << "\n";
      cout << "\tTotal samples run " << trials
           << " times circularly: " << sampleNum * trials << "\n";

///////// Linear Model train ////////
#ifdef BIAS
      Eigen::MatrixXd X3(sampleNum, 4); /* JIA 3 bias */
      Eigen::VectorXd PT_model_3(4);
      Eigen::VectorXd DBT_model_3(4);
      Eigen::VectorXd KSet_model_3(4);
      Eigen::VectorXd DT_model_3(4);
      Eigen::VectorXd MT_model_3(4);

      Eigen::MatrixXd X5(sampleNum, 6); /* JIA 5 bias */
      Eigen::VectorXd PT_model_5(6);
      Eigen::VectorXd DBT_model_5(6);
      Eigen::VectorXd KSet_model_5(6);
      Eigen::VectorXd DT_model_5(6);
      Eigen::VectorXd MT_model_5(6);

      Eigen::MatrixXd X11(sampleNum, 12); /* JIA 11 bias */
      Eigen::VectorXd PT_model_11(12);
      Eigen::VectorXd DBT_model_11(12);
      Eigen::VectorXd KSet_model_11(12);
      Eigen::VectorXd DT_model_11(12);
      Eigen::VectorXd MT_model_11(12);
#else
      // 3維模型
      Eigen::MatrixXd X3(sampleNum, 3);  // 3 features
      Eigen::VectorXd PT_model_3(3);
      Eigen::VectorXd DBT_model_3(3);
      Eigen::VectorXd KSet_model_3(3);
      Eigen::VectorXd DT_model_3(3);
      Eigen::VectorXd MT_model_3(3);

      // 5維模型
      Eigen::MatrixXd X5(sampleNum, 5);  // 5 features
      Eigen::VectorXd PT_model_5(5);
      Eigen::VectorXd DBT_model_5(5);
      Eigen::VectorXd KSet_model_5(5);
      Eigen::VectorXd DT_model_5(5);
      Eigen::VectorXd MT_model_5(5);

      // 11維模型
      Eigen::MatrixXd X11(sampleNum, 11);  // 11 features
      Eigen::VectorXd PT_model_11(11);
      Eigen::VectorXd DBT_model_11(11);
      Eigen::VectorXd KSet_model_11(11);
      Eigen::VectorXd DT_model_11(11);
      Eigen::VectorXd MT_model_11(11);
#endif

      // double x_source_ip = 0, x_destination_ip = 0;
      double x_source_port = 0, x_destination_port = 0, x_protocol = 0;
      double x_source_ip_0 = 0, x_source_ip_1 = 0, x_source_ip_2 = 0,
             x_source_ip_3 = 0;
      double x_destination_ip_0 = 0, x_destination_ip_1 = 0,
             x_destination_ip_2 = 0, x_destination_ip_3 = 0;
      for (size_t i = 0; i < sampleNum; ++i) {
        // 特徵轉換
        // x_source_ip = ip_to_uint32_be(PT_samples[i].source_ip);
        // x_destination_ip = ip_to_uint32_be(PT_samples[i].destination_ip);
        ////
        extract_ip_bytes_to_float(PT_samples[i].source_ip, ip_bytes);
        x_source_ip_0 = static_cast<double>(ip_bytes[0]);
        x_source_ip_1 = static_cast<double>(ip_bytes[1]);
        x_source_ip_2 = static_cast<double>(ip_bytes[2]);
        x_source_ip_3 = static_cast<double>(ip_bytes[3]);

        extract_ip_bytes_to_float(PT_samples[i].destination_ip, ip_bytes);
        x_destination_ip_0 = static_cast<double>(ip_bytes[0]);
        x_destination_ip_1 = static_cast<double>(ip_bytes[1]);
        x_destination_ip_2 = static_cast<double>(ip_bytes[2]);
        x_destination_ip_3 = static_cast<double>(ip_bytes[3]);
        x_source_port = static_cast<double>(PT_samples[i].source_port);
        x_destination_port =
            static_cast<double>(PT_samples[i].destination_port);
        x_protocol = static_cast<double>(PT_samples[i].protocol);

        // 填入3維特徵
        X3(i, 0) = x_source_ip_0;
        X3(i, 1) = x_source_ip_1;
        X3(i, 2) = x_destination_ip_0;
#ifdef BIAS
        X3(i, 3) = 1.0;  // bias
#endif
        // 填入5維特徵
        X5(i, 0) = x_source_ip_0;
        X5(i, 1) = x_source_ip_1;
        X5(i, 2) = x_destination_ip_0;
        X5(i, 3) = x_destination_ip_1;
        X5(i, 4) = x_destination_ip_2;
#ifdef BIAS
        X5(i, 5) = 1.0;  // bias
#endif
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
#ifdef BIAS
        X11(i, 11) = 1.0;  // bias
#endif
      }
#ifdef NORM
      Eigen::VectorXd mean_X3, std_X3;
      Eigen::VectorXd mean_X5, std_X5;
      Eigen::VectorXd mean_X11, std_X11;
      /* JIA normalizeFeatures */ normalizeFeatures(X3, mean_X3, std_X3);
      /* JIA normalizeFeatures */ normalizeFeatures(X5, mean_X5, std_X5);
      /* JIA normalizeFeatures */ normalizeFeatures(X11, mean_X11, std_X11);
#endif
      // 模型擬合
      PT_model_3 = linearRegressionFit(X3, PT_y);
      PT_model_5 = linearRegressionFit(X5, PT_y);
      PT_model_11 = linearRegressionFit(X11, PT_y);
      DBT_model_3 = linearRegressionFit(X3, DBT_y);
      DBT_model_5 = linearRegressionFit(X5, DBT_y);
      DBT_model_11 = linearRegressionFit(X11, DBT_y);
      KSet_model_3 = linearRegressionFit(X3, KSet_y);
      KSet_model_5 = linearRegressionFit(X5, KSet_y);
      KSet_model_11 = linearRegressionFit(X11, KSet_y);
      DT_model_3 = linearRegressionFit(X3, DT_y);
      DT_model_5 = linearRegressionFit(X5, DT_y);
      DT_model_11 = linearRegressionFit(X11, DT_y);
      MT_model_3 = linearRegressionFit(X3, MT_y);
      MT_model_5 = linearRegressionFit(X5, MT_y);
      MT_model_11 = linearRegressionFit(X11, MT_y);

      // 輸出模型參數
      cout << "\n========= Model parameters =========\n";
      cout << "\n[PT 3-feature model] (x1, x2, x3):\n"
           << PT_model_3.transpose() << "\n";
      cout << "\n[PT 5-feature model] (x1~x5):\n"
           << PT_model_5.transpose() << "\n";
      cout << "\n[PT 11-feature model] (x1~x11):\n"
           << PT_model_11.transpose() << "\n";
      cout << "\n[DBT 3-feature model] (x1, x2, x3):\n"
           << DBT_model_3.transpose() << "\n";
      cout << "\n[DBT 5-feature model] (x1~x5):\n"
           << DBT_model_5.transpose() << "\n";
      cout << "\n[DBT 11-feature model] (x1~x11):\n"
           << DBT_model_11.transpose() << "\n";
      cout << "\n[KSet 3-feature model] (x1, x2, x3):\n"
           << KSet_model_3.transpose() << "\n";
      cout << "\n[KSet 5-feature model] (x1~x5):\n"
           << KSet_model_5.transpose() << "\n";
      cout << "\n[KSet 11-feature model] (x1~x11):\n"
           << KSet_model_11.transpose() << "\n";
      cout << "\n[DT 3-feature model] (x1, x2, x3):\n"
           << DT_model_3.transpose() << "\n";
      cout << "\n[DT 5-feature model] (x1~x5):\n"
           << DT_model_5.transpose() << "\n";
      cout << "\n[DT 11-feature model] (x1~x11):\n"
           << DT_model_11.transpose() << "\n";
      cout << "\n[MT 3-feature model] (x1, x2, x3):\n"
           << MT_model_3.transpose() << "\n";
      cout << "\n[MT 5-feature model] (x1~x5):\n"
           << MT_model_5.transpose() << "\n";
      cout << "\n[MT 11-feature model] (x1~x11):\n"
           << MT_model_11.transpose() << "\n";
      ///////// Linear Model train ////////

      /////// -th ///////
      /* auto [mean_PT, median_PT, per75_PT, per95_PT, per99_PT] = */
      printStatistics(PT_y);
      /* auto [mean_DBT, median_DBT, per75_DBT, per95_DBT, per99_DBT] = */
      printStatistics(DBT_y);
      /* auto [mean_KSet, median_KSet, per75_KSet, per95_KSet, per99_KSet] = */
      printStatistics(KSet_y);
      /* auto [mean_DT, median_DT, per75_DT, per95_DT, per99_DT] = */
      printStatistics(DT_y);
      /* auto [mean_MT, median_MT, per75_MT, per95_MT, per99_MT] = */
      printStatistics(MT_y);
      /////// -th ///////

      /////// Linear Model ACC ///////
      {
        FILE *total_prediction_3_out =
            fopen("./INFO/Total_prediction_3_result.txt", "w");
        if (!total_prediction_3_out) {
          cerr << "Cannot open Total_prediction_3_result.txt: "
               << strerror(errno) << '\n';
          return EXIT_FAILURE;
        }

        int model_acc = 0, model_fail = 0, model_oth = 0;
        for (size_t i = 0; i < sampleNum; ++i) {
          // //  source IP（big-endian 轉 uint32）
          // x_source_ip = ip_to_uint32_be(PT_samples[i].source_ip);
          // //  destination IP（big-endian 轉 uint32）
          // x_destination_ip = ip_to_uint32_be(PT_samples[i].destination_ip);

          ////
          extract_ip_bytes_to_float(PT_samples[i].source_ip, ip_bytes);
          x_source_ip_0 = static_cast<double>(ip_bytes[0]);
          x_source_ip_1 = static_cast<double>(ip_bytes[1]);

          extract_ip_bytes_to_float(PT_samples[i].destination_ip, ip_bytes);
          x_destination_ip_0 = static_cast<double>(ip_bytes[0]);
#ifdef NORM
          /* JIA normalizeFeatures */
          double x1_norm_3 = toNormalized(x_source_ip_0, mean_X3[0], std_X3[0]);
          double x2_norm_3 = toNormalized(x_source_ip_1, mean_X3[1], std_X3[1]);
          double x3_norm_3 =
              toNormalized(x_destination_ip_0, mean_X3[2], std_X3[2]);

          //// PT
          double predicted_time_3_pt =
              predict3(PT_model_3, x1_norm_3, x2_norm_3, x3_norm_3);
#else
          /* JIA non-normalizeFeatures */
          double predicted_time_3_pt = predict3(
              PT_model_3, x_source_ip_0, x_source_ip_1, x_destination_ip_0);
#endif
          //// PT

#ifdef NORM
          //// DBT
          double predicted_time_3_dbt =
              predict3(DBT_model_3, x1_norm_3, x2_norm_3, x3_norm_3);
#else
          /* JIA non-normalizeFeatures */
          double predicted_time_3_dbt = predict3(
              DBT_model_3, x_source_ip_0, x_source_ip_1, x_destination_ip_0);
#endif
          //// DBT

#ifdef NORM
          //// KSet
          double predicted_time_3_kset =
              predict3(KSet_model_3, x1_norm_3, x2_norm_3, x3_norm_3);
#else
          /* JIA non-normalizeFeatures */
          double predicted_time_3_kset = predict3(
              KSet_model_3, x_source_ip_0, x_source_ip_1, x_destination_ip_0);
#endif
          //// KSet

#ifdef NORM
          //// DT
          double predicted_time_3_dt =
              predict3(DT_model_3, x1_norm_3, x2_norm_3, x3_norm_3);
#else
          /* JIA non-normalizeFeatures */
          double predicted_time_3_dt = predict3(
              DT_model_3, x_source_ip_0, x_source_ip_1, x_destination_ip_0);
#endif
          //// DT

#ifdef NORM
          //// MT
          double predicted_time_3_mt =
              predict3(MT_model_3, x1_norm_3, x2_norm_3, x3_norm_3);
#else
          /* JIA non-normalizeFeatures */
          double predicted_time_3_mt = predict3(
              MT_model_3, x_source_ip_0, x_source_ip_1, x_destination_ip_0);
#endif
          //// MT

          //// acc
          auto [min_val, min_id_predict] = get_min_max_time(
              predicted_time_3_pt, predicted_time_3_dbt, predicted_time_3_kset,
              predicted_time_3_dt, predicted_time_3_mt);

          // 建立實際值陣列，方便比較與存取
          array<double, 5> real_values = {PT_y(i), DBT_y(i), KSet_y(i), DT_y(i),
                                          MT_y(i)};

          // 找出實際最小值與最大值的模型索引
          // 找所有最小與最大值的 index
          auto real_min_labels = find_all_min_indices(real_values);
          auto real_max_labels = find_all_max_indices(real_values);

          // 實際的最小與最大值
          double real_min_val = real_values[real_min_labels[0]];
          double real_max_val = real_values[real_max_labels[0]];

          // 輸出 log 訊息
          fprintf(total_prediction_3_out,
                  "PT %.4f\tDBT %.4f\tKSet %.4f\tDT %.4f\tMT %.4f\tmin "
                  "%.4f\tmin_id_predict %d\tMAX %.4f\n",
                  PT_y(i), DBT_y(i), KSet_y(i), DT_y(i), MT_y(i), real_min_val,
                  min_id_predict, real_max_val);

          // 判斷預測結果屬於哪一類
          if (std::find(real_min_labels.begin(), real_min_labels.end(),
                        min_id_predict) != real_min_labels.end()) {
            ++model_acc;  // 預測成功（在所有實際最小值中）
          } else if (std::find(real_max_labels.begin(), real_max_labels.end(),
                               min_id_predict) != real_max_labels.end()) {
            ++model_fail;  // 預測錯誤（選到實際最大值）
          } else {
            ++model_oth;  // 其他情況
          }
          //// acc
        }
        fclose(total_prediction_3_out);
        cout << "\n**************** Model(Acc and Fail) ****************\n";
        cout << "    model_acc 3 (%): " << 100 * model_acc / (sampleNum * 1.0)
             << "\n";
        cout << "    model_fail 3 (%): " << 100 * model_fail / (sampleNum * 1.0)
             << "\n";
        cout << "    model_oth 3 (%): " << 100 * model_oth / (sampleNum * 1.0)
             << "\n";

        FILE *total_prediction_5_out =
            fopen("./INFO/Total_prediction_5_result.txt", "w");
        if (!total_prediction_5_out) {
          cerr << "Cannot open Total_prediction_5_result.txt: "
               << strerror(errno) << '\n';
          return EXIT_FAILURE;
        }

        model_acc = 0;
        model_fail = 0;
        model_oth = 0;
        for (size_t i = 0; i < sampleNum; ++i) {
          extract_ip_bytes_to_float(PT_samples[i].source_ip, ip_bytes);
          x_source_ip_0 = static_cast<double>(ip_bytes[0]);
          x_source_ip_1 = static_cast<double>(ip_bytes[1]);

          extract_ip_bytes_to_float(PT_samples[i].destination_ip, ip_bytes);
          x_destination_ip_0 = static_cast<double>(ip_bytes[0]);
          x_destination_ip_1 = static_cast<double>(ip_bytes[1]);
          x_destination_ip_2 = static_cast<double>(ip_bytes[2]);

#ifdef NORM
          /* JIA normalizeFeatures */
          double x1_norm_5 = toNormalized(x_source_ip_0, mean_X5[0], std_X5[0]);
          double x2_norm_5 = toNormalized(x_source_ip_1, mean_X5[1], std_X5[1]);
          double x3_norm_5 =
              toNormalized(x_destination_ip_0, mean_X5[2], std_X5[2]);
          double x4_norm_5 =
              toNormalized(x_destination_ip_1, mean_X5[3], std_X5[3]);
          double x5_norm_5 =
              toNormalized(x_destination_ip_2, mean_X5[4], std_X5[4]);

          //// PT
          double predicted_time_5_pt =
              predict5(PT_model_5, x1_norm_5, x2_norm_5, x3_norm_5, x4_norm_5,
                       x5_norm_5);
          //// PT

          //// DBT
          double predicted_time_5_dbt =
              predict5(DBT_model_5, x1_norm_5, x2_norm_5, x3_norm_5, x4_norm_5,
                       x5_norm_5);
          //// DBT

          //// KSet
          double predicted_time_5_kset =
              predict5(KSet_model_5, x1_norm_5, x2_norm_5, x3_norm_5, x4_norm_5,
                       x5_norm_5);
          //// KSet

          //// DT
          double predicted_time_5_dt =
              predict5(DT_model_5, x1_norm_5, x2_norm_5, x3_norm_5, x4_norm_5,
                       x5_norm_5);
          //// DT

          //// MT
          double predicted_time_5_mt =
              predict5(MT_model_5, x1_norm_5, x2_norm_5, x3_norm_5, x4_norm_5,
                       x5_norm_5);
          //// MT
#else
          //// PT
          double predicted_time_5_pt = predict5(
              PT_model_5, x_source_ip_0, x_source_ip_1, x_destination_ip_0,
              x_destination_ip_1, x_destination_ip_2);
          //// PT
          //// DBT
          double predicted_time_5_dbt = predict5(
              DBT_model_5, x_source_ip_0, x_source_ip_1, x_destination_ip_0,
              x_destination_ip_1, x_destination_ip_2);
          //// DBT
          //// KSet
          double predicted_time_5_kset = predict5(
              KSet_model_5, x_source_ip_0, x_source_ip_1, x_destination_ip_0,
              x_destination_ip_1, x_destination_ip_2);
          //// KSet
          //// DT
          double predicted_time_5_dt = predict5(
              DT_model_5, x_source_ip_0, x_source_ip_1, x_destination_ip_0,
              x_destination_ip_1, x_destination_ip_2);
          //// DT
          //// MT
          double predicted_time_5_mt = predict5(
              MT_model_5, x_source_ip_0, x_source_ip_1, x_destination_ip_0,
              x_destination_ip_1, x_destination_ip_2);
          //// MT
#endif
          //// acc
          auto [min_val, min_id_predict] = get_min_max_time(
              predicted_time_5_pt, predicted_time_5_dbt, predicted_time_5_kset,
              predicted_time_5_dt, predicted_time_5_mt);

          // 建立實際值陣列，方便比較與存取
          array<double, 5> real_values = {PT_y(i), DBT_y(i), KSet_y(i), DT_y(i),
                                          MT_y(i)};

          // 找出實際最小值與最大值的模型索引
          // 找所有最小與最大值的 index
          auto real_min_labels = find_all_min_indices(real_values);
          auto real_max_labels = find_all_max_indices(real_values);

          // 實際的最小與最大值
          double real_min_val = real_values[real_min_labels[0]];
          double real_max_val = real_values[real_max_labels[0]];

          // 輸出 log 訊息
          fprintf(total_prediction_5_out,
                  "PT %.4f\tDBT %.4f\tKSet %.4f\tDT %.4f\tMT %.4f\tmin "
                  "%.4f\tmin_id_predict %d\tMAX %.4f\n",
                  PT_y(i), DBT_y(i), KSet_y(i), DT_y(i), MT_y(i), real_min_val,
                  min_id_predict, real_max_val);

          // 判斷預測結果屬於哪一類
          if (std::find(real_min_labels.begin(), real_min_labels.end(),
                        min_id_predict) != real_min_labels.end()) {
            ++model_acc;  // 預測成功（在所有實際最小值中）
          } else if (std::find(real_max_labels.begin(), real_max_labels.end(),
                               min_id_predict) != real_max_labels.end()) {
            ++model_fail;  // 預測錯誤（選到實際最大值）
          } else {
            ++model_oth;  // 其他情況
          }
          //// acc
        }
        fclose(total_prediction_5_out);
        cout << "    model_acc 5 (%): " << 100 * model_acc / (sampleNum * 1.0)
             << "\n";
        cout << "    model_fail 5 (%): " << 100 * model_fail / (sampleNum * 1.0)
             << "\n";
        cout << "    model_oth 5 (%): " << 100 * model_oth / (sampleNum * 1.0)
             << "\n";

        FILE *total_prediction_11_out =
            fopen("./INFO/Total_prediction_11_result.txt", "w");
        if (!total_prediction_11_out) {
          cerr << "Cannot open Total_prediction_11_result.txt: "
               << strerror(errno) << '\n';
          return EXIT_FAILURE;
        }

        model_acc = 0;
        model_fail = 0;
        model_oth = 0;
        for (size_t i = 0; i < sampleNum; ++i) {
          extract_ip_bytes_to_float(PT_samples[i].source_ip, ip_bytes);
          x_source_ip_0 = static_cast<double>(ip_bytes[0]);
          x_source_ip_1 = static_cast<double>(ip_bytes[1]);
          x_source_ip_2 = static_cast<double>(ip_bytes[2]);
          x_source_ip_3 = static_cast<double>(ip_bytes[3]);

          extract_ip_bytes_to_float(PT_samples[i].destination_ip, ip_bytes);
          x_destination_ip_0 = static_cast<double>(ip_bytes[0]);
          x_destination_ip_1 = static_cast<double>(ip_bytes[1]);
          x_destination_ip_2 = static_cast<double>(ip_bytes[2]);
          x_destination_ip_3 = static_cast<double>(ip_bytes[3]);

          // source port（轉 uint16）
          x_source_port = static_cast<double>(PT_samples[i].source_port);
          x_destination_port =
              static_cast<double>(PT_samples[i].destination_port);
          x_protocol = static_cast<double>(PT_samples[i].protocol);

#ifdef NORM
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

          //// DT
          double predicted_time_11_dt =
              predict11(DT_model_11, x1_norm_11, x2_norm_11, x3_norm_11,
                        x4_norm_11, x5_norm_11, x6_norm_11, x7_norm_11,
                        x8_norm_11, x9_norm_11, x10_norm_11, x11_norm_11);
          //// DT

          //// MT
          double predicted_time_11_mt =
              predict11(MT_model_11, x1_norm_11, x2_norm_11, x3_norm_11,
                        x4_norm_11, x5_norm_11, x6_norm_11, x7_norm_11,
                        x8_norm_11, x9_norm_11, x10_norm_11, x11_norm_11);
          //// MT
#else
          //// PT
          double predicted_time_11_pt = predict11(
              PT_model_11, x_source_ip_0, x_source_ip_1, x_source_ip_2,
              x_source_ip_3, x_destination_ip_0, x_destination_ip_1,
              x_destination_ip_2, x_destination_ip_3, x_source_port,
              x_destination_port, x_protocol);
          //// PT
          //// DBT
          double predicted_time_11_dbt = predict11(
              DBT_model_11, x_source_ip_0, x_source_ip_1, x_source_ip_2,
              x_source_ip_3, x_destination_ip_0, x_destination_ip_1,
              x_destination_ip_2, x_destination_ip_3, x_source_port,
              x_destination_port, x_protocol);
          //// DBT
          //// KSet
          double predicted_time_11_kset = predict11(
              KSet_model_11, x_source_ip_0, x_source_ip_1, x_source_ip_2,
              x_source_ip_3, x_destination_ip_0, x_destination_ip_1,
              x_destination_ip_2, x_destination_ip_3, x_source_port,
              x_destination_port, x_protocol);
          //// KSet
          //// DT
          double predicted_time_11_dt = predict11(
              DT_model_11, x_source_ip_0, x_source_ip_1, x_source_ip_2,
              x_source_ip_3, x_destination_ip_0, x_destination_ip_1,
              x_destination_ip_2, x_destination_ip_3, x_source_port,
              x_destination_port, x_protocol);
          //// DT
          //// MT
          double predicted_time_11_mt = predict11(
              MT_model_11, x_source_ip_0, x_source_ip_1, x_source_ip_2,
              x_source_ip_3, x_destination_ip_0, x_destination_ip_1,
              x_destination_ip_2, x_destination_ip_3, x_source_port,
              x_destination_port, x_protocol);
          //// MT
#endif
          //// acc
          auto [min_val, min_id_predict] =
              get_min_max_time(predicted_time_11_pt, predicted_time_11_dbt,
                               predicted_time_11_kset, predicted_time_11_dt,
                               predicted_time_11_mt);

          // 建立實際值陣列，方便比較與存取
          array<double, 5> real_values = {PT_y(i), DBT_y(i), KSet_y(i), DT_y(i),
                                          MT_y(i)};

          // 找出實際最小值與最大值的模型索引
          // 找所有最小與最大值的 index
          auto real_min_labels = find_all_min_indices(real_values);
          auto real_max_labels = find_all_max_indices(real_values);

          // 實際的最小與最大值
          double real_min_val = real_values[real_min_labels[0]];
          double real_max_val = real_values[real_max_labels[0]];

          // 輸出 log 訊息
          fprintf(total_prediction_11_out,
                  "PT %.4f\tDBT %.4f\tKSet %.4f\tDT %.4f\tMT %.4f\tmin "
                  "%.4f\tmin_id_predict %d\tMAX %.4f\n",
                  PT_y(i), DBT_y(i), KSet_y(i), DT_y(i), MT_y(i), real_min_val,
                  min_id_predict, real_max_val);

          // 判斷預測結果屬於哪一類
          if (std::find(real_min_labels.begin(), real_min_labels.end(),
                        min_id_predict) != real_min_labels.end()) {
            ++model_acc;  // 預測成功（在所有實際最小值中）
          } else if (std::find(real_max_labels.begin(), real_max_labels.end(),
                               min_id_predict) != real_max_labels.end()) {
            ++model_fail;  // 預測錯誤（選到實際最大值）
          } else {
            ++model_oth;  // 其他情況
          }
          //// acc
        }
        fclose(total_prediction_11_out);
        cout << "    model_acc 11 (%): " << 100 * model_acc / (sampleNum * 1.0)
             << "\n";
        cout << "    model_fail 11 (%): "
             << 100 * model_fail / (sampleNum * 1.0) << "\n";
        cout << "    model_oth 11 (%): " << 100 * model_oth / (sampleNum * 1.0)
             << "\n";
      }
      /////// Linear Model ACC ///////

      cout << ("\n**************** Classification(Model) ****************\n");
      int model_counter_DBT = 0, model_counter_PT = 0, model_counter_KSet = 0,
          model_counter_DT = 0, model_counter_MT = 0;
      Omp_predict_time = 0;
      Sig_predict_time = 0;
      Total_search_time = 0;
#ifdef PERLOOKUPTIME_MODEL
      FILE *total_model_3_fp = nullptr;
      total_model_3_fp = fopen("./INFO/Total_model_3_result.txt", "w");
      FILE *total_model_5_fp = nullptr;
      total_model_5_fp = fopen("./INFO/Total_model_5_result.txt", "w");
      FILE *total_model_11_fp = nullptr;
      total_model_11_fp = fopen("./INFO/Total_model_11_result.txt", "w");
#endif
      //// Linear Model 3-D Predict and Search
      {
#pragma omp parallel
        {
#pragma omp for schedule(static)
          for (size_t i = 0; i < 10; ++i) {
            float ip_bytes[4];
            // 提取 IP 特徵
            extract_ip_bytes_to_float(PT_packets[i].source_ip, ip_bytes);
            double x1 = static_cast<double>(ip_bytes[0]);
            double x2 = static_cast<double>(ip_bytes[1]);
            extract_ip_bytes_to_float(PT_packets[i].destination_ip, ip_bytes);
            double x3 = static_cast<double>(ip_bytes[0]);
#ifdef NORM
            // 標準化
            double x1n = toNormalized(x1, mean_X3[0], std_X3[0]);
            double x2n = toNormalized(x2, mean_X3[1], std_X3[1]);
            double x3n = toNormalized(x3, mean_X3[2], std_X3[2]);
            // 模型預測
            double t0 = predict3(PT_model_3, x1n, x2n, x3n);
            double t1 = predict3(DBT_model_3, x1n, x2n, x3n);
            double t2 = predict3(KSet_model_3, x1n, x2n, x3n);
            double t3 = predict3(DT_model_3, x1n, x2n, x3n);
            double t4 = predict3(MT_model_3, x1n, x2n, x3n);
#else
            // 模型預測（未標準化）
            double t0 = predict3(PT_model_3, x1, x2, x3);
            double t1 = predict3(DBT_model_3, x1, x2, x3);
            double t2 = predict3(KSet_model_3, x1, x2, x3);
            double t3 = predict3(DT_model_3, x1, x2, x3);
            double t4 = predict3(MT_model_3, x1, x2, x3);
#endif
            // 找出預測最小的模型
            int min_idx = 0;
            double min_val = t0;
            if (t1 < min_val) {
              min_val = t1;
              min_idx = 1;
            }
            if (t2 < min_val) {
              min_val = t2;
              min_idx = 2;
            }
            if (t3 < min_val) {
              min_val = t3;
              min_idx = 3;
            }
            if (t4 < min_val) {
              min_val = t4;
              min_idx = 4;
            }
          }
        }
#if TIMER_METHOD == TIMER_RDTSCP
        Timer::warmup();
#endif
        timer.timeReset();
#pragma omp parallel
        {
          int local_PT = 0, local_DBT = 0, local_KSet = 0, local_DT = 0,
              local_MT = 0;
#ifdef THREAD_NUM
          // 印出每個 thread ID 及總 thread 數
#pragma omp single
          {
            std::cout << "Model-3 Using " << omp_get_num_threads()
                      << " threads.";
          }
#endif
#pragma omp for schedule(static)
          for (size_t i = 0; i < packetNum; ++i) {
            float ip_bytes[4];
            // 提取 IP 特徵
            extract_ip_bytes_to_float(PT_packets[i].source_ip, ip_bytes);
            double x1 = static_cast<double>(ip_bytes[0]);
            double x2 = static_cast<double>(ip_bytes[1]);
            extract_ip_bytes_to_float(PT_packets[i].destination_ip, ip_bytes);
            double x3 = static_cast<double>(ip_bytes[0]);

#ifdef NORM
            // 標準化
            double x1n = toNormalized(x1, mean_X3[0], std_X3[0]);
            double x2n = toNormalized(x2, mean_X3[1], std_X3[1]);
            double x3n = toNormalized(x3, mean_X3[2], std_X3[2]);

            // 模型預測
            double t0 = predict3(PT_model_3, x1n, x2n, x3n);
            double t1 = predict3(DBT_model_3, x1n, x2n, x3n);
            double t2 = predict3(KSet_model_3, x1n, x2n, x3n);
            double t3 = predict3(DT_model_3, x1n, x2n, x3n);
            double t4 = predict3(MT_model_3, x1n, x2n, x3n);
#else
            // 模型預測（未標準化）
            double t0 = predict3(PT_model_3, x1, x2, x3);
            double t1 = predict3(DBT_model_3, x1, x2, x3);
            double t2 = predict3(KSet_model_3, x1, x2, x3);
            double t3 = predict3(DT_model_3, x1, x2, x3);
            double t4 = predict3(MT_model_3, x1, x2, x3);
#endif
            // 找出預測最小的模型
            int min_idx = 0;
            double min_val = t0;
            if (t1 < min_val) {
              min_val = t1;
              min_idx = 1;
            }
            if (t2 < min_val) {
              min_val = t2;
              min_idx = 2;
            }
            if (t3 < min_val) {
              min_val = t3;
              min_idx = 3;
            }
            if (t4 < min_val) {
              min_val = t4;
              min_idx = 4;
            }

            // 根據最小索引累加 local 計數器
            switch (min_idx) {
              case 0:
                ++local_PT;
                break;
              case 1:
                ++local_DBT;
                break;
              case 2:
                ++local_KSet;
                break;
              case 3:
                ++local_DT;
                break;
              case 4:
                ++local_MT;
                break;
            }
          }
          // 匯總到全域變數（使用 atomic）
#pragma omp atomic
          model_counter_PT += local_PT;
#pragma omp atomic
          model_counter_DBT += local_DBT;
#pragma omp atomic
          model_counter_KSet += local_KSet;
#pragma omp atomic
          model_counter_DT += local_DT;
#pragma omp atomic
          model_counter_MT += local_MT;
        }
        Omp_predict_time = ((timer.elapsed_ns() / packetNum));  // 平行處理

        timer.timeReset();
        for (size_t i = 0; i < packetNum; ++i) {
          float ip_bytes[4];
          // 提取 IP 特徵
          extract_ip_bytes_to_float(PT_packets[i].source_ip, ip_bytes);
          double x1 = static_cast<double>(ip_bytes[0]);
          double x2 = static_cast<double>(ip_bytes[1]);

          extract_ip_bytes_to_float(PT_packets[i].destination_ip, ip_bytes);
          double x3 = static_cast<double>(ip_bytes[0]);

#ifdef NORM
          // 標準化
          double x1n = toNormalized(x1, mean_X3[0], std_X3[0]);
          double x2n = toNormalized(x2, mean_X3[1], std_X3[1]);
          double x3n = toNormalized(x3, mean_X3[2], std_X3[2]);

          double t0 = predict3(PT_model_3, x1n, x2n, x3n);
          double t1 = predict3(DBT_model_3, x1n, x2n, x3n);
          double t2 = predict3(KSet_model_3, x1n, x2n, x3n);
          double t3 = predict3(DT_model_3, x1n, x2n, x3n);
          double t4 = predict3(MT_model_3, x1n, x2n, x3n);
#else
          double t0 = predict3(PT_model_3, x1, x2, x3);
          double t1 = predict3(DBT_model_3, x1, x2, x3);
          double t2 = predict3(KSet_model_3, x1, x2, x3);
          double t3 = predict3(DT_model_3, x1, x2, x3);
          double t4 = predict3(MT_model_3, x1, x2, x3);
#endif

          // 比較預測結果，選最小
          int min_idx = 0;
          double min_val = t0;
          if (t1 < min_val) {
            min_val = t1;
            min_idx = 1;
          }
          if (t2 < min_val) {
            min_val = t2;
            min_idx = 2;
          }
          if (t3 < min_val) {
            min_val = t3;
            min_idx = 3;
          }
          if (t4 < min_val) {
            min_val = t4;
            min_idx = 4;
          }
          predict_choose[i] = min_idx;
        }
        Sig_predict_time = ((timer.elapsed_ns() / packetNum));

        // warmup_KSet(set, packets, packetNum, num_set,max_pri_set);
        // warmup_MT(multilayertuple, traces_DT_MT);
        // warmup_PT(tree, PT_packets);
        warmup_DBT(dbt, DBT_packets);
        warmup_DT(dynamictuple, traces_DT_MT);
#if TIMER_METHOD == TIMER_RDTSCP
        Timer::warmup();
#endif
        for (size_t t = 0; t < trials; ++t) {
          for (size_t i = 0; i < packetNum; ++i) {
            timer.timeReset();
            switch (predict_choose[i]) {
              case 0:
                tree.search(PT_packets[i]);
                break;
              case 1:
                dbt.search(DBT_packets[i]);
                break;
              case 2:
                if (__builtin_expect(num_set[0] > 0, 1))
                  kset_match_pri = set0.ClassifyAPacket(packets[i]);
                if (__builtin_expect(
                        kset_match_pri < max_pri_set[1] && num_set[1] > 0, 1))
                  kset_match_pri =
                      max(kset_match_pri, set1.ClassifyAPacket(packets[i]));
                if (__builtin_expect(
                        kset_match_pri < max_pri_set[2] && num_set[2] > 0, 1))
                  kset_match_pri =
                      max(kset_match_pri, set2.ClassifyAPacket(packets[i]));
                if (__builtin_expect(
                        kset_match_pri < max_pri_set[3] && num_set[3] > 0, 0))
                  kset_match_pri =
                      max(kset_match_pri, set3.ClassifyAPacket(packets[i]));
                break;
              case 3:
                (dynamictuple.Lookup(traces_DT_MT[i], 0));
                break;
              case 4:
                (multilayertuple.Lookup(traces_DT_MT[i], 0));
                break;
            }
            _Total_search_time = timer.elapsed_ns();
            Total_search_time += _Total_search_time;
#ifdef PERLOOKUPTIME_MODEL
            Total_y(i) = (static_cast<double>(_Total_search_time));
#endif
          }
        }
        cout << "\n|=== AVG predict time(Model-3  Single): "
             << (Sig_predict_time) << "ns\n======";
        cout << "\n|=== AVG predict time(Model-3  Omp): " << (Omp_predict_time)
             << "ns\n";
        cout << "|=== AVG search with predict time(Model-3 + Omp): "
             << ((Total_search_time / (packetNum * trials)) + Omp_predict_time)
             << "ns\n";
        cout << "|=== PT, DBT, KSET, DT, MT (%): "
             << 100 * (model_counter_PT) / (packetNum * 1.0) << ", "
             << 100 * (model_counter_DBT) / (packetNum * 1.0) << ", "
             << 100 * (model_counter_KSet) / (packetNum * 1.0) << ", "
             << 100 * (model_counter_DT) / (packetNum * 1.0) << ", "
             << 100 * (model_counter_MT) / (packetNum * 1.0) << "\n";
#ifdef PERLOOKUPTIME_MODEL
        for (size_t i = 0; i < packetNum; ++i) {
          fprintf(total_model_3_fp, "Packet %ld \t Time(ns) %f\n", i,
                  Total_y(i));
        }
        fclose(total_model_3_fp);
        printStatistics("Model-3_y", Total_y);
        Total_y = Eigen::VectorXd::Zero(packetNum);
#endif
      }
      //// Linear Model 3-D Predict and Search

      //// Linear Model 5-D Predict and Search
      {
        model_counter_DBT = 0;
        model_counter_PT = 0;
        model_counter_KSet = 0;
        model_counter_DT = 0;
        model_counter_MT = 0;
        Omp_predict_time = 0;
        Sig_predict_time = 0;
        Total_search_time = 0;
#if TIMER_METHOD == TIMER_RDTSCP
        Timer::warmup();
#endif
        timer.timeReset();
#pragma omp parallel
        {
          int local_PT = 0, local_DBT = 0, local_KSet = 0, local_DT = 0,
              local_MT = 0;
#ifdef THREAD_NUM
          // 印出每個 thread ID 及總 thread 數
#pragma omp single
          {
            std::cout << "Model-5 Using " << omp_get_num_threads()
                      << " threads.";
          }
#endif
#pragma omp for schedule(static)
          for (size_t i = 0; i < packetNum; ++i) {
            float ip_bytes[4];

            // Source IP 轉換
            extract_ip_bytes_to_float(PT_packets[i].source_ip, ip_bytes);
            double x1 = static_cast<double>(ip_bytes[0]);
            double x2 = static_cast<double>(ip_bytes[1]);

            // Destination IP 轉換
            extract_ip_bytes_to_float(PT_packets[i].destination_ip, ip_bytes);
            double x5 = static_cast<double>(ip_bytes[0]);
            double x6 = static_cast<double>(ip_bytes[1]);
            double x10 = static_cast<double>(ip_bytes[2]);

#ifdef NORM
            // 標準化
            double x1n = toNormalized(x1, mean_X5[0], std_X5[0]);
            double x2n = toNormalized(x2, mean_X5[1], std_X5[1]);
            double x5n = toNormalized(x5, mean_X5[2], std_X5[2]);
            double x6n = toNormalized(x6, mean_X5[3], std_X5[3]);
            double x10n = toNormalized(x10, mean_X5[4], std_X5[4]);
            // 五個模型預測時間
            double t0 = predict5(PT_model_5, x1n, x2n, x5n, x6n, x10n);
            double t1 = predict5(DBT_model_5, x1n, x2n, x5n, x6n, x10n);
            double t2 = predict5(KSet_model_5, x1n, x2n, x5n, x6n, x10n);
            double t3 = predict5(DT_model_5, x1n, x2n, x5n, x6n, x10n);
            double t4 = predict5(MT_model_5, x1n, x2n, x5n, x6n, x10n);
#else
            // 模型預測時間
            double t0 = predict5(PT_model_5, x1, x2, x5, x6, x10);
            double t1 = predict5(DBT_model_5, x1, x2, x5, x6, x10);
            double t2 = predict5(KSet_model_5, x1, x2, x5, x6, x10);
            double t3 = predict5(DT_model_5, x1, x2, x5, x6, x10);
            double t4 = predict5(MT_model_5, x1, x2, x5, x6, x10);
#endif

            // 找最小值索引
            int min_idx = 0;
            double min_val = t0;
            if (t1 < min_val) {
              min_val = t1;
              min_idx = 1;
            }
            if (t2 < min_val) {
              min_val = t2;
              min_idx = 2;
            }
            if (t3 < min_val) {
              min_val = t3;
              min_idx = 3;
            }
            if (t4 < min_val) {
              min_val = t4;
              min_idx = 4;
            }

            // 累加對應 local counter
            switch (min_idx) {
              case 0:
                ++local_PT;
                break;
              case 1:
                ++local_DBT;
                break;
              case 2:
                ++local_KSet;
                break;
              case 3:
                ++local_DT;
                break;
              case 4:
                ++local_MT;
                break;
            }
          }

          // 匯總到全域變數
#pragma omp atomic
          model_counter_PT += local_PT;
#pragma omp atomic
          model_counter_DBT += local_DBT;
#pragma omp atomic
          model_counter_KSet += local_KSet;
#pragma omp atomic
          model_counter_DT += local_DT;
#pragma omp atomic
          model_counter_MT += local_MT;
        }
        Omp_predict_time = ((timer.elapsed_ns() / packetNum));  // 平行處理

        timer.timeReset();
        for (size_t i = 0; i < packetNum; ++i) {
          float ip_bytes[4];

          // Source IP 轉換
          extract_ip_bytes_to_float(PT_packets[i].source_ip, ip_bytes);
          double x1 = static_cast<double>(ip_bytes[0]);
          double x2 = static_cast<double>(ip_bytes[1]);

          // Destination IP 轉換
          extract_ip_bytes_to_float(PT_packets[i].destination_ip, ip_bytes);
          double x5 = static_cast<double>(ip_bytes[0]);
          double x6 = static_cast<double>(ip_bytes[1]);
          double x10 = static_cast<double>(ip_bytes[2]);

          // 特徵標準化
          double x1n = toNormalized(x1, mean_X5[0], std_X5[0]);
          double x2n = toNormalized(x2, mean_X5[1], std_X5[1]);
          double x5n = toNormalized(x5, mean_X5[2], std_X5[2]);
          double x6n = toNormalized(x6, mean_X5[3], std_X5[3]);
          double x10n = toNormalized(x10, mean_X5[4], std_X5[4]);

          // 五個模型預測時間
          double t0 = predict5(PT_model_5, x1n, x2n, x5n, x6n, x10n);
          double t1 = predict5(DBT_model_5, x1n, x2n, x5n, x6n, x10n);
          double t2 = predict5(KSet_model_5, x1n, x2n, x5n, x6n, x10n);
          double t3 = predict5(DT_model_5, x1n, x2n, x5n, x6n, x10n);
          double t4 = predict5(MT_model_5, x1n, x2n, x5n, x6n, x10n);

          // 找出預測時間最小的模型索引
          int min_idx = 0;
          double min_val = t0;
          if (t1 < min_val) {
            min_val = t1;
            min_idx = 1;
          }
          if (t2 < min_val) {
            min_val = t2;
            min_idx = 2;
          }
          if (t3 < min_val) {
            min_val = t3;
            min_idx = 3;
          }
          if (t4 < min_val) {
            min_val = t4;
            min_idx = 4;
          }

          predict_choose[i] = min_idx;
        }
        Sig_predict_time = ((timer.elapsed_ns() / packetNum));

        // warmup_KSet(set, packets, packetNum, num_set,max_pri_set);
        // warmup_MT(multilayertuple, traces_DT_MT);
        // warmup_PT(tree, PT_packets);
        warmup_DBT(dbt, DBT_packets);
        warmup_DT(dynamictuple, traces_DT_MT);
#if TIMER_METHOD == TIMER_RDTSCP
        Timer::warmup();
#endif
        for (size_t t = 0; t < trials; ++t) {
          for (size_t i = 0; i < packetNum; ++i) {
            timer.timeReset();
            switch (predict_choose[i]) {
              case 0:
                tree.search(PT_packets[i]);
                break;
              case 1:
                dbt.search(DBT_packets[i]);
                break;
              case 2:
                if (__builtin_expect(num_set[0] > 0, 1))
                  kset_match_pri = set0.ClassifyAPacket(packets[i]);
                if (__builtin_expect(
                        kset_match_pri < max_pri_set[1] && num_set[1] > 0, 1))
                  kset_match_pri =
                      max(kset_match_pri, set1.ClassifyAPacket(packets[i]));
                if (__builtin_expect(
                        kset_match_pri < max_pri_set[2] && num_set[2] > 0, 1))
                  kset_match_pri =
                      max(kset_match_pri, set2.ClassifyAPacket(packets[i]));
                if (__builtin_expect(
                        kset_match_pri < max_pri_set[3] && num_set[3] > 0, 0))
                  kset_match_pri =
                      max(kset_match_pri, set3.ClassifyAPacket(packets[i]));
                break;
              case 3:
                (dynamictuple.Lookup(traces_DT_MT[i], 0));
                break;
              case 4:
                (multilayertuple.Lookup(traces_DT_MT[i], 0));
                break;
            }
            _Total_search_time = timer.elapsed_ns();
            Total_search_time += _Total_search_time;
#ifdef PERLOOKUPTIME_MODEL
            Total_y(i) = (static_cast<double>(_Total_search_time));
#endif
          }
        }
        cout << "\n|=== AVG predict time(Model-5  Single): "
             << (Sig_predict_time) << "ns\n======";
        cout << "\n|=== AVG predict time(Model-5  Omp): " << (Omp_predict_time)
             << "ns\n";
        cout << "|=== AVG search with predict time(Model-5 + Omp): "
             << ((Total_search_time / (packetNum * trials)) + Omp_predict_time)
             << "ns\n";
        cout << "|=== PT, DBT, KSET, DT, MT (%): "
             << 100 * (model_counter_PT) / (packetNum * 1.0) << ", "
             << 100 * (model_counter_DBT) / (packetNum * 1.0) << ", "
             << 100 * (model_counter_KSet) / (packetNum * 1.0) << ", "
             << 100 * (model_counter_DT) / (packetNum * 1.0) << ", "
             << 100 * (model_counter_MT) / (packetNum * 1.0) << "\n";
#ifdef PERLOOKUPTIME_MODEL
        for (size_t i = 0; i < packetNum; ++i) {
          fprintf(total_model_5_fp, "Packet %ld \t Time(ns) %f\n", i,
                  Total_y(i));
        }
        fclose(total_model_5_fp);
        printStatistics("Model-5_y", Total_y);
        Total_y = Eigen::VectorXd::Zero(packetNum);
#endif
      }
      //// Linear Model 5-D Predict and Search

      //// Linear Model 11-D Predict and Search
      {
        model_counter_DBT = 0;
        model_counter_PT = 0;
        model_counter_KSet = 0;
        model_counter_DT = 0;
        model_counter_MT = 0;
        Omp_predict_time = 0;
        Sig_predict_time = 0;
        Total_search_time = 0;
#if TIMER_METHOD == TIMER_RDTSCP
        Timer::warmup();
#endif
        timer.timeReset();
#pragma omp parallel
        {
          int local_PT = 0, local_DBT = 0, local_KSet = 0, local_DT = 0,
              local_MT = 0;
#ifdef THREAD_NUM
          // 印出每個 thread ID 及總 thread 數
#pragma omp single
          {
            std::cout << "Model-11 Using " << omp_get_num_threads()
                      << " threads.";
          }
#endif
#pragma omp for schedule(static)
          for (size_t i = 0; i < packetNum; ++i) {
            float ip_bytes[4];
            // 提取 IP 特徵（共 11 維）
            extract_ip_bytes_to_float(PT_packets[i].source_ip, ip_bytes);
            double x1 = static_cast<double>(ip_bytes[0]);
            double x2 = static_cast<double>(ip_bytes[1]);
            double x3 = static_cast<double>(ip_bytes[2]);
            double x4 = static_cast<double>(ip_bytes[3]);

            extract_ip_bytes_to_float(PT_packets[i].destination_ip, ip_bytes);
            double x5 = static_cast<double>(ip_bytes[0]);
            double x6 = static_cast<double>(ip_bytes[1]);
            double x7 = static_cast<double>(ip_bytes[2]);
            double x8 = static_cast<double>(ip_bytes[3]);

            double x9 = static_cast<double>(PT_packets[i].source_port);
            double x10 = static_cast<double>(PT_packets[i].destination_port);
            double x11 = static_cast<double>(PT_packets[i].protocol);

#ifdef NORM
            // 標準化處理
            double x1n = toNormalized(x1, mean_X11[0], std_X11[0]);
            double x2n = toNormalized(x2, mean_X11[1], std_X11[1]);
            double x3n = toNormalized(x3, mean_X11[2], std_X11[2]);
            double x4n = toNormalized(x4, mean_X11[3], std_X11[3]);
            double x5n = toNormalized(x5, mean_X11[4], std_X11[4]);
            double x6n = toNormalized(x6, mean_X11[5], std_X11[5]);
            double x7n = toNormalized(x7, mean_X11[6], std_X11[6]);
            double x8n = toNormalized(x8, mean_X11[7], std_X11[7]);
            double x9n = toNormalized(x9, mean_X11[8], std_X11[8]);
            double x10n = toNormalized(x10, mean_X11[9], std_X11[9]);
            double x11n = toNormalized(x11, mean_X11[10], std_X11[10]);

            double t0 = predict11(PT_model_11, x1n, x2n, x3n, x4n, x5n, x6n,
                                  x7n, x8n, x9n, x10n, x11n);
            double t1 = predict11(DBT_model_11, x1n, x2n, x3n, x4n, x5n, x6n,
                                  x7n, x8n, x9n, x10n, x11n);
            double t2 = predict11(KSet_model_11, x1n, x2n, x3n, x4n, x5n, x6n,
                                  x7n, x8n, x9n, x10n, x11n);
            double t3 = predict11(DT_model_11, x1n, x2n, x3n, x4n, x5n, x6n,
                                  x7n, x8n, x9n, x10n, x11n);
            double t4 = predict11(MT_model_11, x1n, x2n, x3n, x4n, x5n, x6n,
                                  x7n, x8n, x9n, x10n, x11n);
#else
            double t0 = predict11(PT_model_11, x1, x2, x3, x4, x5, x6, x7, x8,
                                  x9, x10, x11);
            double t1 = predict11(DBT_model_11, x1, x2, x3, x4, x5, x6, x7, x8,
                                  x9, x10, x11);
            double t2 = predict11(KSet_model_11, x1, x2, x3, x4, x5, x6, x7, x8,
                                  x9, x10, x11);
            double t3 = predict11(DT_model_11, x1, x2, x3, x4, x5, x6, x7, x8,
                                  x9, x10, x11);
            double t4 = predict11(MT_model_11, x1, x2, x3, x4, x5, x6, x7, x8,
                                  x9, x10, x11);
#endif
            int min_idx = 0;
            double min_val = t0;
            if (t1 < min_val) {
              min_val = t1;
              min_idx = 1;
            }
            if (t2 < min_val) {
              min_val = t2;
              min_idx = 2;
            }
            if (t3 < min_val) {
              min_val = t3;
              min_idx = 3;
            }
            if (t4 < min_val) {
              min_val = t4;
              min_idx = 4;
            }

            switch (min_idx) {
              case 0:
                ++local_PT;
                break;
              case 1:
                ++local_DBT;
                break;
              case 2:
                ++local_KSet;
                break;
              case 3:
                ++local_DT;
                break;
              case 4:
                ++local_MT;
                break;
            }
          }
          // 匯總統計至全域變數
#pragma omp atomic
          model_counter_PT += local_PT;
#pragma omp atomic
          model_counter_DBT += local_DBT;
#pragma omp atomic
          model_counter_KSet += local_KSet;
#pragma omp atomic
          model_counter_DT += local_DT;
#pragma omp atomic
          model_counter_MT += local_MT;
        }
        Omp_predict_time = ((timer.elapsed_ns() / packetNum));  // 平行處理

        timer.timeReset();
        for (size_t i = 0; i < packetNum; ++i) {
          float ip_bytes[4];
          // 提取 IP 特徵（共 11 維）
          extract_ip_bytes_to_float(PT_packets[i].source_ip, ip_bytes);
          double x1 = static_cast<double>(ip_bytes[0]);
          double x2 = static_cast<double>(ip_bytes[1]);
          double x3 = static_cast<double>(ip_bytes[2]);
          double x4 = static_cast<double>(ip_bytes[3]);

          extract_ip_bytes_to_float(PT_packets[i].destination_ip, ip_bytes);
          double x5 = static_cast<double>(ip_bytes[0]);
          double x6 = static_cast<double>(ip_bytes[1]);
          double x7 = static_cast<double>(ip_bytes[2]);
          double x8 = static_cast<double>(ip_bytes[3]);

          double x9 = static_cast<double>(PT_packets[i].source_port);
          double x10 = static_cast<double>(PT_packets[i].destination_port);
          double x11 = static_cast<double>(PT_packets[i].protocol);

#ifdef NORM
          // 標準化處理
          double x1n = toNormalized(x1, mean_X11[0], std_X11[0]);
          double x2n = toNormalized(x2, mean_X11[1], std_X11[1]);
          double x3n = toNormalized(x3, mean_X11[2], std_X11[2]);
          double x4n = toNormalized(x4, mean_X11[3], std_X11[3]);
          double x5n = toNormalized(x5, mean_X11[4], std_X11[4]);
          double x6n = toNormalized(x6, mean_X11[5], std_X11[5]);
          double x7n = toNormalized(x7, mean_X11[6], std_X11[6]);
          double x8n = toNormalized(x8, mean_X11[7], std_X11[7]);
          double x9n = toNormalized(x9, mean_X11[8], std_X11[8]);
          double x10n = toNormalized(x10, mean_X11[9], std_X11[9]);
          double x11n = toNormalized(x11, mean_X11[10], std_X11[10]);

          double t0 = predict11(PT_model_11, x1n, x2n, x3n, x4n, x5n, x6n, x7n,
                                x8n, x9n, x10n, x11n);
          double t1 = predict11(DBT_model_11, x1n, x2n, x3n, x4n, x5n, x6n, x7n,
                                x8n, x9n, x10n, x11n);
          double t2 = predict11(KSet_model_11, x1n, x2n, x3n, x4n, x5n, x6n,
                                x7n, x8n, x9n, x10n, x11n);
          double t3 = predict11(DT_model_11, x1n, x2n, x3n, x4n, x5n, x6n, x7n,
                                x8n, x9n, x10n, x11n);
          double t4 = predict11(MT_model_11, x1n, x2n, x3n, x4n, x5n, x6n, x7n,
                                x8n, x9n, x10n, x11n);
#else
          double t0 = predict11(PT_model_11, x1, x2, x3, x4, x5, x6, x7, x8, x9,
                                x10, x11);
          double t1 = predict11(DBT_model_11, x1, x2, x3, x4, x5, x6, x7, x8,
                                x9, x10, x11);
          double t2 = predict11(KSet_model_11, x1, x2, x3, x4, x5, x6, x7, x8,
                                x9, x10, x11);
          double t3 = predict11(DT_model_11, x1, x2, x3, x4, x5, x6, x7, x8, x9,
                                x10, x11);
          double t4 = predict11(MT_model_11, x1, x2, x3, x4, x5, x6, x7, x8, x9,
                                x10, x11);
#endif
          int min_idx = 0;
          double min_val = t0;
          if (t1 < min_val) {
            min_val = t1;
            min_idx = 1;
          }
          if (t2 < min_val) {
            min_val = t2;
            min_idx = 2;
          }
          if (t3 < min_val) {
            min_val = t3;
            min_idx = 3;
          }
          if (t4 < min_val) {
            min_val = t4;
            min_idx = 4;
          }
          predict_choose[i] = min_idx;
        }
        Sig_predict_time = ((timer.elapsed_ns() / packetNum));

        // warmup_KSet(set, packets, packetNum, num_set,max_pri_set);
        // warmup_MT(multilayertuple, traces_DT_MT);
        // warmup_PT(tree, PT_packets);
        warmup_DBT(dbt, DBT_packets);
        warmup_DT(dynamictuple, traces_DT_MT);
#if TIMER_METHOD == TIMER_RDTSCP
        Timer::warmup();
#endif
        for (size_t t = 0; t < trials; ++t) {
          for (size_t i = 0; i < packetNum; ++i) {
            timer.timeReset();
            switch (predict_choose[i]) {
              case 0:
                tree.search(PT_packets[i]);
                break;
              case 1:
                dbt.search(DBT_packets[i]);
                break;
              case 2:
                if (__builtin_expect(num_set[0] > 0, 1))
                  kset_match_pri = set0.ClassifyAPacket(packets[i]);
                if (__builtin_expect(
                        kset_match_pri < max_pri_set[1] && num_set[1] > 0, 1))
                  kset_match_pri =
                      max(kset_match_pri, set1.ClassifyAPacket(packets[i]));
                if (__builtin_expect(
                        kset_match_pri < max_pri_set[2] && num_set[2] > 0, 1))
                  kset_match_pri =
                      max(kset_match_pri, set2.ClassifyAPacket(packets[i]));
                if (__builtin_expect(
                        kset_match_pri < max_pri_set[3] && num_set[3] > 0, 0))
                  kset_match_pri =
                      max(kset_match_pri, set3.ClassifyAPacket(packets[i]));
                break;
              case 3:
                (dynamictuple.Lookup(traces_DT_MT[i], 0));
                break;
              case 4:
                (multilayertuple.Lookup(traces_DT_MT[i], 0));
                break;
            }
            _Total_search_time = timer.elapsed_ns();
            Total_search_time += _Total_search_time;
#ifdef PERLOOKUPTIME_MODEL
            Total_y(i) = (static_cast<double>(_Total_search_time));
#endif
          }
        }
        cout << "\n|=== AVG predict time(Model-11  Single): "
             << (Sig_predict_time) << "ns\n======";
        cout << "\n|=== AVG predict time(Model-11  Omp): " << (Omp_predict_time)
             << "ns\n";
        cout << "|=== AVG search with predict time(Model-11 + Omp): "
             << ((Total_search_time / (packetNum * trials)) + Omp_predict_time)
             << "ns\n";
        cout << "|=== PT, DBT, KSET, DT, MT (%): "
             << 100 * (model_counter_PT) / (packetNum * 1.0) << ", "
             << 100 * (model_counter_DBT) / (packetNum * 1.0) << ", "
             << 100 * (model_counter_KSet) / (packetNum * 1.0) << ", "
             << 100 * (model_counter_DT) / (packetNum * 1.0) << ", "
             << 100 * (model_counter_MT) / (packetNum * 1.0) << "\n";
#ifdef PERLOOKUPTIME_MODEL
        for (size_t i = 0; i < packetNum; ++i) {
          fprintf(total_model_11_fp, "Packet %ld \t Time(ns) %f\n", i,
                  Total_y(i));
        }
        fclose(total_model_11_fp);
        printStatistics("Model-11_y", Total_y);
        Total_y = Eigen::VectorXd::Zero(packetNum);
#endif
      }
      //// Linear Model 11-D Predict and Search
    }
///////// Linear Model /////////
/*************************************************************************/

/*************************************************************************/
///////// KNN  /////////
#ifdef KNN_MODEL
    ///////// KNN Construct /////////
    vector<uint8_t> knnLable(sampleNum);
    vector<LabeledSample> knn_training_data(sampleNum);
#ifdef SAMPLE
    vector<Packet> knn_packets = packets;
    vector<Packet> knn_samples = samples;
    // normalize_first_two_dims(knn_samples);
    // normalize_first_two_dims(knn_packets);
#else
    vector<Packet> knn_packets = packets;
    // normalize_first_two_dims(knn_packets);
    vector<Packet> &knn_samples = packets;
#endif
    for (size_t i = 0; i < sampleNum; ++i) {
      array<double, 5> real_values = {PT_y(i), DBT_y(i), KSet_y(i), DT_y(i),
                                      MT_y(i)};
      auto real_min_labels = find_all_min_indices(real_values);
      knnLable[i] = real_min_labels[0];
      knn_training_data[i] = {knn_samples[i],
                              static_cast<DataStructure>(knnLable[i])};
    }
    KNNClassifier knn_classifier(knn_training_data);
    vector<uint8_t>().swap(knnLable);
    // vector<LabeledSample>().swap(knn_training_data); // WRONG!! (core dumped)
    ///////// KNN Construct /////////
    vector<array<double, 5>> datas;
    convert_packets_to_datas(knn_packets, datas);
    //// KNN ACC
    {
      FILE *knn_prediction_out =
          fopen("./INFO/Total_prediction_knn_result.txt", "w");
      if (!knn_prediction_out) {
        cerr << "Cannot open Total_prediction_knn_result.txt: "
             << strerror(errno) << '\n';
        return EXIT_FAILURE;
      }
      int KNN_acc = 0, KNN_fail = 0, KNN_oth = 0;
      for (size_t i = 0; i < packetNum; ++i) {
        // knn_classifier.predict_vote
        // knn_classifier.predict
        int min_id_predict = static_cast<int>(knn_classifier.predict(datas[i]));
        //// acc
        // 建立實際值陣列，方便比較與存取
        array<double, 5> real_values = {PT_y(i), DBT_y(i), KSet_y(i), DT_y(i),
                                        MT_y(i)};

        // 找出實際最小值與最大值的模型索引
        // 找所有最小與最大值的 index
        auto real_min_labels = find_all_min_indices(real_values);
        auto real_max_labels = find_all_max_indices(real_values);

        // 實際的最小與最大值
        double real_min_val = real_values[real_min_labels[0]];
        double real_max_val = real_values[real_max_labels[0]];

        // 輸出 log 訊息
        fprintf(knn_prediction_out,
                "PT %.4f\tDBT %.4f\tKSet %.4f\tDT %.4f\tMT %.4f\tmin "
                "%.4f\tmin_id_predict %d\tMAX %.4f\n",
                PT_y(i), DBT_y(i), KSet_y(i), DT_y(i), MT_y(i), real_min_val,
                min_id_predict, real_max_val);

        // 判斷預測結果屬於哪一類
        if (std::find(real_min_labels.begin(), real_min_labels.end(),
                      min_id_predict) != real_min_labels.end()) {
          ++KNN_acc;  // 預測成功（在所有實際最小值中）
        } else if (std::find(real_max_labels.begin(), real_max_labels.end(),
                             min_id_predict) != real_max_labels.end()) {
          ++KNN_fail;  // 預測錯誤（選到實際最大值）
        } else {
          ++KNN_oth;  // 其他情況
        }
        //// acc
      }
      fclose(knn_prediction_out);
      cout << "\n**************** KNN(Acc and Fail) ****************\n";
      cout << "    KNN_acc (%): " << 100 * KNN_acc / (packetNum * 1.0) << "\n";
      cout << "    KNN_fail (%): " << 100 * KNN_fail / (packetNum * 1.0)
           << "\n";
      cout << "    KNN_oth (%): " << 100 * KNN_oth / (packetNum * 1.0) << "\n";
      cout << "**************** KNN(Acc and Fail) ****************\n";
    }
    //// KNN ACC

    //// KNN Predict and Search
    {
      Omp_predict_time = 0;
      Sig_predict_time = 0;
      int knn_counter_DBT = 0, knn_counter_PT = 0, knn_counter_KSet = 0,
          knn_counter_DT = 0, knn_counter_MT = 0;
#if TIMER_METHOD == TIMER_RDTSCP
      Timer::warmup();
#endif
      timer.timeReset();
#pragma omp parallel
      {
        int local_PT = 0, local_DBT = 0, local_KSet = 0, local_DT = 0,
            local_MT = 0;
#ifdef THREAD_NUM
        // 印出每個 thread ID 及總 thread 數
#pragma omp single
        {
          std::cout << "KNN Using " << omp_get_num_threads() << " threads.";
        }
#endif
#pragma omp for schedule(static)
        for (size_t i = 0; i < packetNum; ++i) {
          // 根據最小索引累加 local 計數器
          switch (static_cast<int>(knn_classifier.predict(datas[i]))) {
            case 0:
              ++local_PT;
              break;
            case 1:
              ++local_DBT;
              break;
            case 2:
              ++local_KSet;
              break;
            case 3:
              ++local_DT;
              break;
            case 4:
              ++local_MT;
              break;
          }
        }
        // 匯總到全域變數（使用 atomic）
#pragma omp atomic
        knn_counter_PT += local_PT;
#pragma omp atomic
        knn_counter_DBT += local_DBT;
#pragma omp atomic
        knn_counter_KSet += local_KSet;
#pragma omp atomic
        knn_counter_DT += local_DT;
#pragma omp atomic
        knn_counter_MT += local_MT;
      }
      Omp_predict_time = ((timer.elapsed_ns() / packetNum));  // 平行處理

      timer.timeReset();
      for (size_t i = 0; i < packetNum; ++i) {
        predict_choose[i] = static_cast<int>(knn_classifier.predict(datas[i]));
      }
      Sig_predict_time = ((timer.elapsed_ns() / packetNum));

#ifdef PERLOOKUPTIME_KNN
      FILE *total_knn_fp = nullptr;
      total_knn_fp = fopen("./INFO/Total_knn_result.txt", "w");
#endif
      // warmup_KSet(set, packets, packetNum, num_set,max_pri_set);
      // warmup_MT(multilayertuple, traces_DT_MT);
      // warmup_PT(tree, PT_packets);
      warmup_DBT(dbt, DBT_packets);
      warmup_DT(dynamictuple, traces_DT_MT);
#if TIMER_METHOD == TIMER_RDTSCP
      Timer::warmup();
#endif
      for (size_t t = 0; t < trials; ++t) {
        for (size_t i = 0; i < packetNum; ++i) {
          timer.timeReset();
          switch (predict_choose[i]) {
            case 0:
              tree.search(PT_packets[i]);
              break;
            case 1:
              dbt.search(DBT_packets[i]);
              break;
            case 2:
              if (__builtin_expect(num_set[0] > 0, 1))
                kset_match_pri = set0.ClassifyAPacket(packets[i]);
              if (__builtin_expect(
                      kset_match_pri < max_pri_set[1] && num_set[1] > 0, 1))
                kset_match_pri =
                    max(kset_match_pri, set1.ClassifyAPacket(packets[i]));
              if (__builtin_expect(
                      kset_match_pri < max_pri_set[2] && num_set[2] > 0, 1))
                kset_match_pri =
                    max(kset_match_pri, set2.ClassifyAPacket(packets[i]));
              if (__builtin_expect(
                      kset_match_pri < max_pri_set[3] && num_set[3] > 0, 0))
                kset_match_pri =
                    max(kset_match_pri, set3.ClassifyAPacket(packets[i]));
              break;
            case 3:
              (dynamictuple.Lookup(traces_DT_MT[i], 0));
              break;
            case 4:
              (multilayertuple.Lookup(traces_DT_MT[i], 0));
              break;
          }
          _Total_search_time = timer.elapsed_ns();
          Total_search_time += _Total_search_time;
#ifdef PERLOOKUPTIME_KNN
          Total_y(i) = (static_cast<double>(_Total_search_time));
#endif
        }
      }
      cout << "\n|=== AVG predict time(KNN  Single): " << (Sig_predict_time)
           << "ns\n======";
      cout << "\n|=== AVG predict time(KNN  Omp): " << (Omp_predict_time)
           << "ns\n";
      cout << "|=== AVG search with predict time(KNN + Omp): "
           << ((Total_search_time / (packetNum * trials)) + Omp_predict_time)
           << "ns\n";
      cout << "|=== PT, DBT, KSET, DT, MT (%): "
           << 100 * (knn_counter_PT) / (packetNum * 1.0) << ", "
           << 100 * (knn_counter_DBT) / (packetNum * 1.0) << ", "
           << 100 * (knn_counter_KSet) / (packetNum * 1.0) << ", "
           << 100 * (knn_counter_DT) / (packetNum * 1.0) << ", "
           << 100 * (knn_counter_MT) / (packetNum * 1.0) << "\n";
#ifdef PERLOOKUPTIME_KNN
      for (size_t i = 0; i < packetNum; ++i) {
        fprintf(total_knn_fp, "Packet %ld \t Time(ns) %f\n", i, Total_y(i));
      }
      fclose(total_knn_fp);
      printStatistics("KNN_y", Total_y);
      Total_y = Eigen::VectorXd::Zero(packetNum);
#endif
    }
#endif
    //// KNN Predict and Search
    ///////// KNN  /////////
    /*************************************************************************/

    /*************************************************************************/
    ///////// BloomFilter Construct /////////

#ifdef BLOOM
    {
      cout << ("\n**************** Classification(BLOOM) ****************\n");
      XAI::BloomFilter<uint64_t> bloom_filter_mt(sampleNum * 0.5, 0.01);
      XAI::BloomFilter<uint64_t> bloom_filter_dt(sampleNum * 0.5, 0.01);
      XAI::BloomFilter<uint64_t> bloom_filter_pt(sampleNum * 0.5, 0.01);
      XAI::BloomFilter<uint64_t> bloom_filter_dbt(sampleNum * 0.5, 0.01);

      auto [mean_PT, median_PT, per75_PT, per95_PT, per99_PT] =
          printStatistics(PT_y);
      auto [mean_DBT, median_DBT, per75_DBT, per95_DBT, per99_DBT] =
          printStatistics(DBT_y);
      auto [mean_DT, median_DT, per75_DT, per95_DT, per99_DT] =
          printStatistics(DT_y);
      auto [mean_MT, median_MT, per75_MT, per95_MT, per99_MT] =
          printStatistics(MT_y);

      for (size_t i = 0; i < sampleNum; ++i) {
        if (PT_y(i) >= per99_PT) {
          bloom_filter_pt.insert(
              (PT_samples[i].toIP64()) ^
              (((static_cast<uint64_t>(PT_samples[i].source_port) << 16) |
                static_cast<uint64_t>(PT_samples[i].destination_port))
               << 17));
        }
        if (DBT_y(i) >= per99_DBT) {
          bloom_filter_dbt.insert(
              (DBT_samples[i].ip.i_64) ^
              (((static_cast<uint64_t>(DBT_samples[i].Port[0]) << 16) |
                static_cast<uint64_t>(DBT_samples[i].Port[1]))
               << 17));
        }
        if (DT_y(i) >= per99_DT) {
          bloom_filter_dt.insert(
              (DT_MT_samples[i]->dst_src_ip) ^
              (((static_cast<uint64_t>(DT_MT_samples[i]->key[2]) << 16) |
                static_cast<uint64_t>(DT_MT_samples[i]->key[3]))
               << 17));
        }
        if (MT_y(i) >= per99_MT) {
          bloom_filter_mt.insert(
              (DT_MT_samples[i]->dst_src_ip) ^
              (((static_cast<uint64_t>(DT_MT_samples[i]->key[2]) << 16) |
                static_cast<uint64_t>(DT_MT_samples[i]->key[3]))
               << 17));
        }
      }
      ///////// BloomFilter Construct /////////
      /*************************************************************************/

      /*************************************************************************/
      ///////// BloomFilter Classification /////////
      FILE *Bloom_res = nullptr;
      Bloom_res = fopen("./INFO/BloomResults.txt", "w");

      Omp_predict_time = 0;
      Sig_predict_time = 0;
      Total_search_time = 0;
      int bloom_counter_DBT = 0;
      int bloom_counter_PT = 0;
      int bloom_counter_KSet = 0;
      int bloom_counter_DT = 0;
      int bloom_counter_MT = 0;
#if TIMER_METHOD == TIMER_RDTSCP
      Timer::warmup();
#endif
      timer.timeReset();
#pragma omp parallel
      {
        int local_bloom_DBT = 0;
        int local_bloom_PT = 0;
        int local_bloom_DT = 0;
        int local_bloom_MT = 0;
        int local_bloom_KSet = 0;
#ifdef THREAD_NUM
        // 印出每個 thread ID 及總 thread 數
#pragma omp single
        {
          std::cout << "Bloom Using " << omp_get_num_threads() << " threads.";
        }
#endif
#pragma omp for schedule(static)
        for (size_t i = 0; i < packetNum; ++i) {
          const uint64_t key_dbt =
              (DBT_packets[i].ip.i_64) ^
              (((static_cast<uint64_t>(DBT_packets[i].Port[0]) << 16) |
                static_cast<uint64_t>(DBT_packets[i].Port[1]))
               << 17);

          const uint64_t key_pt =
              (PT_packets[i].toIP64()) ^
              (((static_cast<uint64_t>(PT_packets[i].source_port) << 16) |
                static_cast<uint64_t>(PT_packets[i].destination_port))
               << 17);

          const uint64_t key_dt_mt =
              (traces_DT_MT[i]->dst_src_ip) ^
              (((static_cast<uint64_t>(traces_DT_MT[i]->key[2]) << 16) |
                static_cast<uint64_t>(traces_DT_MT[i]->key[3]))
               << 17);
          const bool hit_dbt = bloom_filter_dbt.contains(key_dbt);
          const bool hit_pt = bloom_filter_pt.contains(key_pt);
          const bool hit_dt = bloom_filter_dt.contains(key_dt_mt);
          const bool hit_mt = bloom_filter_mt.contains(key_dt_mt);

          if (!hit_dbt) {
            ++local_bloom_DBT;
            continue;
          }
          if (!hit_dt) {
            ++local_bloom_DT;
            continue;
          }
          if (!hit_pt) {
            ++local_bloom_PT;
            continue;
          }
          if (!hit_mt) {
            ++local_bloom_MT;
            continue;
          }
          ++local_bloom_KSet;
        }

#pragma omp atomic
        bloom_counter_DBT += local_bloom_DBT;
#pragma omp atomic
        bloom_counter_PT += local_bloom_PT;
#pragma omp atomic
        bloom_counter_DT += local_bloom_DT;
#pragma omp atomic
        bloom_counter_MT += local_bloom_MT;
#pragma omp atomic
        bloom_counter_KSet += local_bloom_KSet;
      }
      Omp_predict_time = ((timer.elapsed_ns() / packetNum));  // 平行處理

      timer.timeReset();
      for (size_t i = 0; i < packetNum; ++i) {
        // 預先計算 Bloom key（避免重複呼叫 XOR 運算）
        const uint64_t key_dbt =
            (DBT_packets[i].ip.i_64) ^
            (((static_cast<uint64_t>(DBT_packets[i].Port[0]) << 16) |
              static_cast<uint64_t>(DBT_packets[i].Port[1]))
             << 17);

        const uint64_t key_pt =
            (PT_packets[i].toIP64()) ^
            (((static_cast<uint64_t>(PT_packets[i].source_port) << 16) |
              static_cast<uint64_t>(PT_packets[i].destination_port))
             << 17);

        const uint64_t key_dt_mt =
            (traces_DT_MT[i]->dst_src_ip) ^
            (((static_cast<uint64_t>(traces_DT_MT[i]->key[2]) << 16) |
              static_cast<uint64_t>(traces_DT_MT[i]->key[3]))
             << 17);

        // Bloom Filter 查詢（避免 else-if 串鍊）
        const bool hit_dbt = bloom_filter_dbt.contains(key_dbt);
        const bool hit_pt = bloom_filter_pt.contains(key_pt);
        const bool hit_dt = bloom_filter_dt.contains(key_dt_mt);
        const bool hit_mt = bloom_filter_mt.contains(key_dt_mt);
        if (!hit_dbt) {
          predict_choose[i] = 1;
          continue;
        }
        if (!hit_dt) {
          predict_choose[i] = 3;
          continue;
        }
        if (!hit_pt) {
          predict_choose[i] = 0;
          continue;
        }
        if (!hit_mt) {
          predict_choose[i] = 4;
          continue;
        }
        predict_choose[i] = 2;
      }
      Sig_predict_time = ((timer.elapsed_ns() / packetNum));

      // warmup_KSet(set, packets, packetNum, num_set,max_pri_set);
      // warmup_MT(multilayertuple, traces_DT_MT);
      // warmup_PT(tree, PT_packets);
      warmup_DBT(dbt, DBT_packets);
      warmup_DT(dynamictuple, traces_DT_MT);
#if TIMER_METHOD == TIMER_RDTSCP
      Timer::warmup();
#endif
      for (size_t t = 0; t < trials; ++t) {
        for (size_t i = 0; i < packetNum; ++i) {
          timer.timeReset();
          switch (predict_choose[i]) {
            case 0:
              tree.search(PT_packets[i]);
              break;
            case 1:
              dbt.search(DBT_packets[i]);
              break;
            case 2:
              kset_match_pri = -1;
              if (__builtin_expect(num_set[0] > 0, 1))
                kset_match_pri = set0.ClassifyAPacket(packets[i]);
              if (__builtin_expect(
                      kset_match_pri < max_pri_set[1] && num_set[1] > 0, 1))
                kset_match_pri =
                    max(kset_match_pri, set1.ClassifyAPacket(packets[i]));
              if (__builtin_expect(
                      kset_match_pri < max_pri_set[2] && num_set[2] > 0, 1))
                kset_match_pri =
                    max(kset_match_pri, set2.ClassifyAPacket(packets[i]));
              if (__builtin_expect(
                      kset_match_pri < max_pri_set[3] && num_set[3] > 0, 0))
                kset_match_pri =
                    max(kset_match_pri, set3.ClassifyAPacket(packets[i]));
              break;
            case 3:
              (dynamictuple.Lookup(traces_DT_MT[i], 0));
              break;
            case 4:
              (multilayertuple.Lookup(traces_DT_MT[i], 0));
              break;
          }
          _Total_search_time = timer.elapsed_ns();
          Total_search_time += _Total_search_time;
          Total_y(i) = (static_cast<double>(_Total_search_time));
        }
      }
      cout << "\n|=== AVG predict time(BloomFilter  Single): "
           << (Sig_predict_time) << "ns\n======";
      cout << "\n|=== AVG predict time(BloomFilter  Omp): "
           << (Omp_predict_time) << "ns\n";
      cout << "|=== AVG search time with predict(BloomFilter + Omp): "
           << ((Total_search_time / (packetNum * trials)) + Omp_predict_time)
           << "ns\n";
      cout << "|=== PT, DBT, KSET, DT, MT (%): "
           << 100 * (bloom_counter_PT) / (packetNum * 1.0) << ", "
           << 100 * (bloom_counter_DBT) / (packetNum * 1.0) << ", "
           << 100 * (bloom_counter_KSet) / (packetNum * 1.0) << ", "
           << 100 * (bloom_counter_DT) / (packetNum * 1.0) << ", "
           << 100 * (bloom_counter_MT) / (packetNum * 1.0) << "\n";

      for (size_t i = 0; i < packetNum; ++i) {
        fprintf(Bloom_res, "Packet %ld \t Time(ns) %f\n", i, Total_y(i));
      }
      fclose(Bloom_res);
      printStatistics("Total_y", Total_y);

      ///////// BloomFilter Classification /////////
      /*************************************************************************/
    }
#endif
    /*************************************************************************/
    ///////// Individual /////////
    {
      // Eigen::VectorXd PT_y(packetNum);
      // Eigen::VectorXd DBT_y(packetNum);
      // Eigen::VectorXd KSet_y(packetNum);
      // Eigen::VectorXd DT_y(packetNum);
      // Eigen::VectorXd MT_y(packetNum);
      vector<int> PT_match_id_arr(packetNum);
      vector<uint32_t> DBT_match_id_arr(packetNum);
      vector<int> KSet_match_pri_arr(packetNum);
      vector<int> DT_match_id_arr(packetNum);
      vector<int> MT_match_id_arr(packetNum);

      PT_search_time = 0;
      DBT_search_time = 0;
      KSet_search_time = 0;
      DT_search_time = 0;
      MT_search_time = 0;
      cout << ("\n************** Classification(Individual) **************");
      // PT classification
      cout << ("\n**************** Classification(PT) ****************\n");
      FILE *PT_res_fp = nullptr;
      PT_res_fp = fopen("./INFO/PT_IndivResults.txt", "w");

      // warmup_PT(tree, PT_packets, packetNum);
      for (size_t t = 0; t < trials; ++t) {
        for (size_t i = 0; i < packetNum; ++i) {
          timer.timeReset();
          (PT_match_id = tree.search(PT_packets[i]))--;
          _PT_search_time = timer.elapsed_ns();
          PT_search_time += _PT_search_time;

          Total_y(i) = (static_cast<double>(_PT_search_time));
          PT_match_id_arr[i] = PT_match_id;
        }
      }

      for (size_t i = 0; i < packetNum; ++i) {
        fprintf(PT_res_fp, "Packet %ld \t Time(ns) %f\n", i, Total_y(i));
      }
      fclose(PT_res_fp);
      printStatistics("indiv_PT_y", Total_y);
      cout << "|- Average search time: "
           << PT_search_time / (packetNum * trials) << "ns\n";
      //// PT ////

      // DBT classification
      cout << ("\n**************** Classification(DBT) ****************\n");
      FILE *DBT_res_fp = nullptr;
      DBT_res_fp = fopen("./INFO/DBT_IndivResults.txt", "w");

      // warmup_DBT(dbt, DBT_packets, packetNum);
      for (size_t t = 0; t < trials; ++t) {
        for (size_t i = 0; i < packetNum; ++i) {
          timer.timeReset();
          (DBT_match_id = dbt.search(DBT_packets[i]))--;
          _DBT_search_time = timer.elapsed_ns();
          DBT_search_time += _DBT_search_time;
          Total_y(i) = (static_cast<double>(_DBT_search_time));
          DBT_match_id_arr[i] = DBT_match_id;
        }
      }

      for (size_t i = 0; i < packetNum; ++i) {
        fprintf(DBT_res_fp, "Packet %ld \t Time(ns) %f\n", i, Total_y(i));
      }
      fclose(DBT_res_fp);
      printStatistics("indiv_DBT_y", Total_y);

      cout << "|- Average search time: "
           << DBT_search_time / (packetNum * trials) << "ns\n";
      dbt.search_with_log(DBT_packets);
      //// DBT ////

      //// KSet ////
      cout << ("\n**************** Classification(KSet) ****************\n");
      FILE *KSet_res_fp = nullptr;
      KSet_res_fp = fopen("./INFO/KSet_IndivResults.txt", "w");

      warmup_KSet(set, packets, num_set, max_pri_set);
#if TIMER_METHOD == TIMER_RDTSCP
      Timer::warmup();
#endif
      for (size_t t = 0; t < trials; ++t) {
        for (size_t i = 0; i < packetNum; ++i) {
          const auto &ptk = packets[i];
          kset_match_pri = -1;
          timer.timeReset();
          if (__builtin_expect(num_set[0] > 0, 1))
            kset_match_pri = set0.ClassifyAPacket(ptk);
          if (__builtin_expect(
                  kset_match_pri < max_pri_set[1] && num_set[1] > 0, 1))
            kset_match_pri = max(kset_match_pri, set1.ClassifyAPacket(ptk));
          if (__builtin_expect(
                  kset_match_pri < max_pri_set[2] && num_set[2] > 0, 1))
            kset_match_pri = max(kset_match_pri, set2.ClassifyAPacket(ptk));
          if (__builtin_expect(
                  kset_match_pri < max_pri_set[3] && num_set[3] > 0, 0))
            kset_match_pri = max(kset_match_pri, set3.ClassifyAPacket(ptk));
          _KSet_search_time = timer.elapsed_ns();
          KSet_search_time += _KSet_search_time;
          Total_y(i) = (static_cast<double>(_KSet_search_time));
          KSet_match_pri_arr[i] = (number_rule - 1) - kset_match_pri;
        }
      }

      for (size_t i = 0; i < packetNum; ++i) {
        fprintf(KSet_res_fp, "Packet %ld \t Time(ns) %f\n", i, Total_y(i));
      }
      fclose(KSet_res_fp);
      printStatistics("indiv_KSet_y", Total_y);

      cout << fixed << setprecision(3) << "\tAverage search time: "
           << (KSet_search_time / (trials * packetNum)) << " ns\n";

      //// DT ////
      cout << ("\n**************** Classification(DT) ****************\n");
      FILE *DT_res_fp = nullptr;
      DT_res_fp = fopen("./INFO/DT_IndivResults.txt", "w");

      warmup_DT(dynamictuple, traces_DT_MT);
      for (size_t t = 0; t < trials; ++t) {
        for (size_t i = 0; i < packetNum; ++i) {
          timer.timeReset();
          auto resID = (dynamictuple.Lookup(traces_DT_MT[i], 0));
          _DT_search_time = timer.elapsed_ns();
          DT_search_time += _DT_search_time;
          Total_y(i) = (static_cast<double>(_DT_search_time));
          DT_match_id_arr[i] = (number_rule - 1) - resID;
        }
      }

      for (size_t i = 0; i < packetNum; ++i) {
        fprintf(DT_res_fp, "Packet %ld \t Time(ns) %f\n", i, Total_y(i));
      }
      fclose(DT_res_fp);
      printStatistics("indiv_DT_y", Total_y);
      cout << fixed << setprecision(3) << "\tAverage search time: "
           << (DT_search_time / (trials * packetNum)) << " ns\n";

      //// DT ////

      //// MT ////
      cout << ("\n**************** Classification(MT) ****************\n");
      FILE *MT_res_fp = nullptr;
      MT_res_fp = fopen("./INFO/MT_IndivResults.txt", "w");

      warmup_MT(multilayertuple, traces_DT_MT);
#if TIMER_METHOD == TIMER_RDTSCP
      Timer::warmup();
#endif
      for (size_t t = 0; t < trials; ++t) {
        for (size_t i = 0; i < packetNum; ++i) {
          timer.timeReset();
          auto resID = (multilayertuple.Lookup(traces_DT_MT[i], 0));
          _MT_search_time = timer.elapsed_ns();
          MT_search_time += _MT_search_time;
          Total_y(i) = (static_cast<double>(_MT_search_time));
          MT_match_id_arr[i] = (number_rule - 1) - resID;
        }
      }

      for (size_t i = 0; i < packetNum; ++i) {
        fprintf(MT_res_fp, "Packet %ld \t Time(ns) %f\n", i, Total_y(i));
      }
      fclose(MT_res_fp);
      printStatistics("indiv_MT_y", Total_y);
      cout << fixed << setprecision(3) << "\tAverage search time: "
           << (MT_search_time / (trials * packetNum)) << " ns\n";

      //// MT ////

#ifdef VALID
      for (size_t i = 0; i < packetNum; ++i) {
        if (KSet_match_pri_arr[i] != PT_match_id_arr[i] ||
            DBT_match_id_arr[i] != PT_match_id_arr[i] ||
            DT_match_id_arr[i] != PT_match_id_arr[i] ||
            MT_match_id_arr[i] != PT_match_id_arr[i])
          cout << i << "-th WRONG\n";
      }
#endif
      // free
      dynamictuple.Free(false);
      multilayertuple.Free(false);
      ///////// Individual /////////
      /*************************************************************************/
    }
  }
  return 0;
}
