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
#include "pt_tree.hpp"
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
#define PERLOOKUPTIME
#define ENABLE_OUTPUT_PREDICTION
#define TOTAL

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
int max_pri_set[4] = {-1, -1, -1, -1};

constexpr int seed = 11;

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
  cout << ("================Precompute=============\n");
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
void bytes_reverse(uint32_t ip, uint8_t bytes[4]) {
  bytes[0] = (ip >> 24) & 0xFF;
  bytes[1] = (ip >> 16) & 0xFF;
  bytes[2] = (ip >> 8) & 0xFF;
  bytes[3] = ip & 0xFF;
}
void bytesAllocate(uint32_t ip, uint8_t bytes[4]) {
  bytes[0] = ip & 0xFF;
  bytes[1] = (ip >> 8) & 0xFF;
  bytes[2] = (ip >> 16) & 0xFF;
  bytes[3] = (ip >> 24) & 0xFF;
}
// ===============================
// Convert Packet -> PT_Packet
// ===============================
std::vector<PT_Packet> convertToPTPackets(const std::vector<Packet> &packets) {
  std::vector<PT_Packet> pt_packets;

  for (const auto &pkt : packets) {
    PT_Packet pt{};
    uint32_t sip = pkt[0];
    uint32_t dip = pkt[1];

    // IP 順序修正：big endian
    // bytes_reverse(sip, pt.source_ip);
    // bytes_reverse(dip, pt.destination_ip);
    bytesAllocate(sip, pt.source_ip);
    bytesAllocate(dip, pt.destination_ip);
    pt.source_port = static_cast<uint16_t>(pkt[2]);
    pt.destination_port = static_cast<uint16_t>(pkt[3]);
    pt.protocol = pkt[4];

    pt_packets.emplace_back(pt);
  }

  return pt_packets;
}

// ===============================
// Convert Rule -> PT_Rule
// ===============================
std::vector<PT_Rule> convertToPTRules(const std::vector<Rule> &rules) {
  std::vector<PT_Rule> pt_rules;

  for (const auto &r : rules) {
    PT_Rule pt{};
    pt.pri = r.pri;

    // protocol
    pt.protocol[0] = (r.range[4][0] == r.range[4][1]) ? 0xFF : 0x00;
    pt.protocol[1] = static_cast<unsigned char>(r.range[4][0]);

    // source IP
    pt.source_mask = static_cast<unsigned char>(r.prefix_length[0]);
    // bytes_reverse(r.range[0][0], pt.source_ip);
    bytesAllocate(r.range[0][0], pt.source_ip);
    // destination IP
    pt.destination_mask = static_cast<unsigned char>(r.prefix_length[1]);
    // bytes_reverse(r.range[1][0], pt.destination_ip);
    bytesAllocate(r.range[1][0], pt.destination_ip);
    // ports
    pt.source_port[0] = static_cast<uint16_t>(r.range[2][0]);
    pt.source_port[1] = static_cast<uint16_t>(r.range[2][1]);
    pt.destination_port[0] = static_cast<uint16_t>(r.range[3][0]);
    pt.destination_port[1] = static_cast<uint16_t>(r.range[3][1]);

    pt_rules.emplace_back(pt);
  }

  return pt_rules;
}

// ===============================
// Export PT_Packet to file
// ===============================
void exportPTPackets(const std::vector<PT_Packet> &packets,
                     const std::string &filename) {
  FILE *fp = fopen(filename.c_str(), "w");
  if (!fp) {
    perror("Failed to open output PT_Packet file");
    return;
  }

  for (const auto &pkt : packets) {
    fprintf(fp, "%u.%u.%u.%u\t", pkt.source_ip[3], pkt.source_ip[2],
            pkt.source_ip[1], pkt.source_ip[0]);
    fprintf(fp, "%u.%u.%u.%u\t", pkt.destination_ip[3], pkt.destination_ip[2],
            pkt.destination_ip[1], pkt.destination_ip[0]);
    fprintf(fp, "%u\t%u\t%u\n", pkt.source_port, pkt.destination_port,
            pkt.protocol);
  }

  fclose(fp);
}

// ===============================
// Export PT_Rule to file
// ===============================
void exportPTRules(const std::vector<PT_Rule> &rules,
                   const std::string &filename) {
  FILE *fp = fopen(filename.c_str(), "w");
  if (!fp) {
    perror("Failed to open output PT_Rule file");
    return;
  }

  for (const auto &r : rules) {
    fprintf(fp, "@%u.%u.%u.%u/%u\t", r.source_ip[3], r.source_ip[2],
            r.source_ip[1], r.source_ip[0], r.source_mask);
    fprintf(fp, "%u.%u.%u.%u/%u\t", r.destination_ip[3], r.destination_ip[2],
            r.destination_ip[1], r.destination_ip[0], r.destination_mask);
    fprintf(fp, "%u : %u\t", r.source_port[0], r.source_port[1]);
    fprintf(fp, "%u : %u\t", r.destination_port[0], r.destination_port[1]);
    fprintf(fp, "%02X/%02X\n", r.protocol[1], r.protocol[0]);
  }

  fclose(fp);
}

std::vector<DBT::Packet> convertToDBTPackets(
    const std::vector<Packet> &packets) {
  std::vector<DBT::Packet> dbt_packets;

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

  return dbt_packets;
}

DBT::vector<DBT::Rule> convertToDBTRules(const std::vector<Rule> &rules) {
  DBT::vector<DBT::Rule> dbt_rules;

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
    dbt_r.ip.i_64 &= dbt_r.mask.i_64;

    dbt_rules.emplace_back(dbt_r);
  }

  return dbt_rules;
}
/////////////////
int main(int argc, char *argv[]) {
  CommandLineParser parser;
  parser.parseArguments(argc, argv);
  vector<Rule> rule, build, update;
  vector<Packet> packets;
  InputFile inputFile;
  InputFile_test inputFile_test;
  Timer timer;
  unsigned long long KSet_search_time = 0, _KSet_search_time = 0;
  constexpr int trials = 2;  // run 10 times circularly

  inputFile.loadRule(rule, parser.getRulesetFile());
  inputFile.loadPacket(packets, parser.getTraceFile());
  if (parser.isTestMode()) {
    Timer timerTest;
    timerTest.timeReset();
    inputFile_test.loadRule_test(rule, LoadRule_test_path);
    cout << "Input rule test time(ns): " << timerTest.elapsed_ns() << "\n";

    timerTest.timeReset();
    inputFile_test.loadPacket_test(packets, LoadPacket_test_path);
    cout << "Input packet test time(ns): " << timerTest.elapsed_ns() << "\n";
  }
  const size_t number_rule = rule.size();
  cout << "The number of rules = " << number_rule << "\n";

  //// PT ////
  auto PT_packets = convertToPTPackets(packets);
  auto PT_rule = convertToPTRules(rule);
  if (parser.isTestMode()) {
    exportPTPackets(PT_packets, "./INFO/PT_packets.txt");
    exportPTRules(PT_rule, "./INFO/PT_rules.txt");
  }
  setmaskHash();
  /***********************************************************************************************************************/
  // search config
  /***********************************************************************************************************************/
  vector<uint8_t> set_field = parser.getField();
  int set_port = parser.getPort();
  if (set_field.size() == 0) {
    timer.timeReset();
    CacuInfo cacu(PT_rule);
    cacu.read_fields();
    set_field = cacu.cacu_best_fields();
    set_port = 1;

    cout << "search config time: " << timer.elapsed_s() << " s" << endl;
  }
  for (int i = 0; i < set_field.size(); ++i) cout << set_field[i] << " ";
  PTtree tree(set_field, set_port);
  const size_t packetNum = PT_packets.size();
  /***********************************************************************************************************************/
  // insert
  /***********************************************************************************************************************/
  cout << "\nStart build for single thread...\n|- Using fields:     ";
  for (unsigned int x : set_field) cout << x << ",";
  cout << set_port << endl;
  timer.timeReset();
  for (auto &&r : PT_rule) {
    tree.insert(r);
  }
  cout << "|- Construct time:   " << timer.elapsed_ns() << "ns\n";
  cout << "|- tree.totalNodes: " << tree.totalNodes << endl;

  cout << "|- Memory footprint: " << (double)tree.mem() / 1024.0 / 1024.0
       << "MB\n";
  /***********************************************************************************************************************/
  // Search
  /***********************************************************************************************************************/
  cout << "\nPT start search...\n";
  int PT_match_id = 0;
#ifdef PERLOOKUPTIME
  FILE *PT_res_fp = nullptr;
  PT_res_fp = fopen("./INFO/PT_results.txt", "w");
#endif
  unsigned long long PT_search_time = 0, _PT_search_time = 0;
  for (int i = 0; i < packetNum; ++i) {
    timer.timeReset();
    PT_match_id = tree.search(PT_packets[i]);
    _PT_search_time = timer.elapsed_ns();
    PT_search_time += _PT_search_time;
    --PT_match_id;
#ifdef PERLOOKUPTIME
    fprintf(PT_res_fp, "Packet %d \t Result %d \t Time(ns) %f\n", i,
            (PT_match_id), _PT_search_time / 1.0);
#endif
  }
#ifdef PERLOOKUPTIME
  fclose(PT_res_fp);
#endif
  cout << "|- Average search time: " << PT_search_time / packetNum << "ns\n";
  //// PT ////

  //// DBT ////
  auto DBT_packets = convertToDBTPackets(packets);
  auto DBT_rules = convertToDBTRules(rule);

  cout << "binth=" << DBT::BINTH << " th_b=" << DBT::END_BOUND
       << " K=" << DBT::TOP_K << " th_c=" << DBT::C_BOUND << endl
       << endl;
  DBT::DBTable dbt(DBT_rules, DBT::BINTH);
  timer.timeReset();
  dbt.construct();
  cout << "Construction Time: " << timer.elapsed_ns() << " ns\n";
  dbt.mem();

  cout << "\nstart search...\n";
  uint32_t DBT_match_id = 0;
#ifdef PERLOOKUPTIME
  FILE *DBT_res_fp = nullptr;
  DBT_res_fp = fopen("./INFO/DBT_results.txt", "w");
#endif
  unsigned long long DBT_search_time = 0, _DBT_search_time = 0;

  for (int i = 0; i < packetNum; ++i) {
    timer.timeReset();
    DBT_match_id = dbt.search(DBT_packets[i]);
    _DBT_search_time = timer.elapsed_ns();
    DBT_search_time += _DBT_search_time;
    --DBT_match_id;
#ifdef PERLOOKUPTIME
    fprintf(DBT_res_fp, "Packet %d \t Result %u \t Time(ns) %f\n", i,
            DBT_match_id, _DBT_search_time / 1.0);
#endif
  }
#ifdef PERLOOKUPTIME
  fclose(DBT_res_fp);
#endif
  cout << "|- Average search time: " << DBT_search_time / packetNum << "ns\n";

  dbt.search_with_log(DBT_packets);
  //// DBT ////

  //// KSet ////
  vector<Rule> set_4[4];

  size_t number_update_rule = 0;
  if (parser.isUpdMode()) {
    srand(seed);
    int randn;

    for (size_t i = 0; i < number_rule; ++i) {
      randn = rand() % 100;
      if (randn < 95)
        build.push_back(rule[i]);
      else
        update.push_back(rule[i]);
    }

    const size_t number_build_rule = build.size();
    number_update_rule = update.size();

    cout << "The number of rules for build = " << number_build_rule << "\n";

    anaK(number_build_rule, build, SetBits, set_4, pre_K);
  } else {
    anaK(number_rule, rule, SetBits, set_4, pre_K);
  }

  int num_set[4] = {0};
  for (size_t i = 0; i < 4; ++i) {
    num_set[i] = set_4[i].size();
  }

  KSet set0(0, set_4[0], SetBits[0]);
  KSet set1(1, set_4[1], SetBits[1]);
  KSet set2(2, set_4[2], SetBits[2]);
  KSet set3(3, set_4[3], 0);
  // construct
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
  set0.prints();
  set1.prints();
  set2.prints();
  set3.prints();
  //// KSet ////

  if (parser.isSearchMode()) {
    // classification
    cout << ("\n**************** Classification(KSet) ****************\n");
    cout << "\tThe number of packet in the trace file = " << packetNum << "\n";
    cout << "\tTotal packets run " << trials
         << "times circularly: " << packetNum * trials << "\n";

    vector<int> matchid(packetNum);
    if (max_pri_set[1] < max_pri_set[2]) max_pri_set[1] = max_pri_set[2];
#ifdef PERLOOKUPTIME
    std::ofstream KSet_per_log("./INFO/KSet_results.txt", std::ios_base::out);
    if (!KSet_per_log) {
      std::cerr << "Error: Failed to open perLookTimes.txt\n";
      return -1;
    }
#endif
    for (size_t t = 0; t < trials; ++t) {
      for (size_t i = 0; i < packetNum; ++i) {
        int match_pri = -1;
        timer.timeReset();
        if (num_set[0] > 0) match_pri = set0.ClassifyAPacket(packets[i]);
        if (match_pri < max_pri_set[1] && num_set[1] > 0)
          match_pri = max(match_pri, set1.ClassifyAPacket(packets[i]));
        if (match_pri < max_pri_set[2] && num_set[2] > 0)
          match_pri = max(match_pri, set2.ClassifyAPacket(packets[i]));
        if (match_pri < max_pri_set[3] && num_set[3] > 0)
          match_pri = max(match_pri, set3.ClassifyAPacket(packets[i]));
        _KSet_search_time = timer.elapsed_ns();
        KSet_search_time += _KSet_search_time;
        matchid[i] = (number_rule - 1) - match_pri;

#ifdef PERLOOKUPTIME
        if (1 == t) {
          KSet_per_log << "Packet " << i << " \t Result " << matchid[i]
                       << " \t Time(ns) " << _KSet_search_time / 1.0 << "\n";
        }
#endif
      }
    }
#ifdef PERLOOKUPTIME
    KSet_per_log.close();
#endif

    cout << fixed << setprecision(3)  // 設定小數點後 3 位
         << "\tTotal classification time: " << (KSet_search_time * trials)
         << " ns" << endl
         << "\tAverage classification time: "
         << (KSet_search_time / (trials * packetNum)) << " ns\n";
  }

  if (parser.isUpdMode()) {
    cout << ("\n**************** Update ****************\n");
    cout << "\tThe number of updated rules = " << number_update_rule << endl;
    int srcmask, dstmask;

    // insert
    unsigned long long KSet_insert_time = 0, _KSet_insert_time = 0;
    for (size_t i = 0; i < number_update_rule; ++i) {
      timer.timeReset();
      srcmask = update[i].prefix_length[0];
      dstmask = update[i].prefix_length[1];
      if ((srcmask >= SetBits[0]) && (dstmask >= SetBits[0]))
        set0.InsertRule(update[i]);
      else if (srcmask >= SetBits[1])
        set1.InsertRule(update[i]);
      else if (dstmask >= SetBits[2])
        set2.InsertRule(update[i]);
      else
        set3.InsertRule(update[i]);
      _KSet_insert_time = timer.elapsed_ns();
      KSet_insert_time += _KSet_insert_time;
    }

    // delete
    unsigned long long KSet_delete_time = 0, _KSet_delete_time = 0;
    for (size_t i = 0; i < number_update_rule; ++i) {
      timer.timeReset();
      srcmask = update[i].prefix_length[0];
      dstmask = update[i].prefix_length[1];
      if ((srcmask >= SetBits[0]) && (dstmask >= SetBits[0]))
        set0.DeleteRule(update[i]);
      else if (srcmask >= SetBits[1])
        set1.DeleteRule(update[i]);
      else if (dstmask >= SetBits[2])
        set2.DeleteRule(update[i]);
      else
        set3.DeleteRule(update[i]);
      _KSet_delete_time = timer.elapsed_ns();
      KSet_delete_time += _KSet_delete_time;
    }

    cout << fixed << setprecision(3)  // 設定小數點後 3 位
         << "\tAverage insert time: " << (KSet_insert_time / number_update_rule)
         << " ns, "
         << "Average delete time: " << (KSet_delete_time / number_update_rule)
         << " ns" << endl
         << "\tAverage update time: "
         << ((KSet_insert_time + KSet_delete_time) / (number_update_rule * 2))
         << " ns" << endl;
  }

////////// =============== /////////////
#ifdef TOTAL
  if (parser.isSearchMode()) {
    unsigned long long Total_search_time = 0, _Total_search_time = 0;
    unsigned long long Total_predict_time = 0, _Total_predict_time = 0;
    // classification
    cout << ("\n**************** Classification(TOTAL) ****************\n");
    cout << "\tThe number of packet in the trace file = " << packetNum << "\n";
    cout << "\tTotal packets run " << trials
         << "times circularly: " << packetNum * trials << "\n";

    if (max_pri_set[1] < max_pri_set[2]) max_pri_set[1] = max_pri_set[2];
    dbt.construct();

    ///////// train ////////
    // 三維模型
    MatrixXd X3(packetNum, 4);  // 3 features + bias
    VectorXd PT_model_3(4);
    VectorXd KSet_model_3(4);
    VectorXd DBT_model_3(4);

    // 11維模型
    MatrixXd X11(packetNum, 12);  // 11 features + bias
    VectorXd PT_model_11(12);
    VectorXd KSet_model_11(12);
    VectorXd DBT_model_11(12);

    // y 向量共用 (3, 11)
    VectorXd PT_y(packetNum);
    VectorXd KSet_y(packetNum);
    VectorXd DBT_y(packetNum);

    double x_source_ip = 0, x_destination_ip = 0;
    double x_source_port = 0, x_destination_port = 0, x_protocol = 0;
    double x_source_ip_0 = 0, x_source_ip_1 = 0, x_source_ip_2 = 0,
           x_source_ip_3 = 0;
    double x_destination_ip_0 = 0, x_destination_ip_1 = 0,
           x_destination_ip_2 = 0, x_destination_ip_3 = 0;
    float out[4] = {0};

    for (int i = 0; i < packetNum; ++i) {
      // 特徵轉換
      // x_source_ip = ip_to_uint32_be(PT_packets[i].source_ip);
      // x_destination_ip = ip_to_uint32_be(PT_packets[i].destination_ip);
      ////
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
      x_destination_port = static_cast<double>(PT_packets[i].destination_port);
      x_protocol = static_cast<double>(PT_packets[i].protocol);

      // 搜尋時間量測 (PT)
      tree.search(PT_packets[i]);
      timer.timeReset();
      tree.search(PT_packets[i]);
      _PT_search_time = timer.elapsed_ns();
      PT_y(i) = static_cast<double>(_PT_search_time);

      // 搜尋時間量測 (KSet)
      int match_pri = -1;
      if (num_set[0] > 0) match_pri = set0.ClassifyAPacket(packets[i]);
      if (match_pri < max_pri_set[1] && num_set[1] > 0)
        match_pri = max(match_pri, set1.ClassifyAPacket(packets[i]));
      if (match_pri < max_pri_set[2] && num_set[2] > 0)
        match_pri = max(match_pri, set2.ClassifyAPacket(packets[i]));
      if (match_pri < max_pri_set[3] && num_set[3] > 0)
        match_pri = max(match_pri, set3.ClassifyAPacket(packets[i]));
      match_pri = -1;
      timer.timeReset();
      if (num_set[0] > 0) match_pri = set0.ClassifyAPacket(packets[i]);
      if (match_pri < max_pri_set[1] && num_set[1] > 0)
        match_pri = max(match_pri, set1.ClassifyAPacket(packets[i]));
      if (match_pri < max_pri_set[2] && num_set[2] > 0)
        match_pri = max(match_pri, set2.ClassifyAPacket(packets[i]));
      if (match_pri < max_pri_set[3] && num_set[3] > 0)
        match_pri = max(match_pri, set3.ClassifyAPacket(packets[i]));
      _KSet_search_time = timer.elapsed_ns();
      KSet_y(i) = static_cast<double>(_KSet_search_time);

      // 搜尋時間量測 (DBT)
      dbt.search(DBT_packets[i]);
      timer.timeReset();
      dbt.search(DBT_packets[i]);
      _DBT_search_time = timer.elapsed_ns();
      DBT_y(i) = static_cast<double>(_DBT_search_time);

      // 填入三維特徵
      X3(i, 0) = x_source_ip_0;
      X3(i, 1) = x_source_ip_1;
      X3(i, 2) = x_destination_ip_0;
      X3(i, 3) = 1.0;
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
      X11(i, 11) = 1.0;
    }
    vector<double> mean_X3, std_X3;
    vector<double> mean_X5, std_X5;
    vector<double> mean_X11, std_X11;

    normalizeFeatures(X3, mean_X3, std_X3);
    normalizeFeatures(X11, mean_X11, std_X11);

    // 模型擬合
    PT_model_3 = linearRegressionFit(X3, PT_y);
    PT_model_11 = linearRegressionFit(X11, PT_y);
    KSet_model_3 = linearRegressionFit(X3, KSet_y);
    KSet_model_11 = linearRegressionFit(X11, KSet_y);
    DBT_model_3 = linearRegressionFit(X3, DBT_y);
    DBT_model_11 = linearRegressionFit(X11, DBT_y);

    // 輸出模型參數
    cout << "\n[PT 3-feature model] (x1, x2, x3, bias):\n"
         << PT_model_3.transpose() << endl;
    cout << "\n[PT 11-feature model] (x1~x11, bias):\n"
         << PT_model_11.transpose() << endl;
    cout << "\n[KSet 3-feature model] (x1, x2, x3, bias):\n"
         << KSet_model_3.transpose() << endl;
    cout << "\n[KSet 11-feature model] (x1~x11, bias):\n"
         << KSet_model_11.transpose() << endl;
    cout << "\n[DBT 3-feature model] (x1, x2, x3, bias):\n"
         << DBT_model_3.transpose() << endl;
    cout << "\n[DBT 11-feature model] (x1~x11, bias):\n"
         << DBT_model_11.transpose() << endl;
    ///////// train ////////

    /////// benchmark ///////
    evaluateModel(X3 * PT_model_3, PT_y, "PT-3-feature");
    evaluateModel(X11 * PT_model_11, PT_y, "PT-11-feature");
    evaluateModel(X3 * KSet_model_3, KSet_y, "KSet-3-feature");
    evaluateModel(X11 * KSet_model_11, KSet_y, "KSet-11-feature");
    evaluateModel(X3 * DBT_model_3, DBT_y, "DBT-3-feature");
    evaluateModel(X11 * DBT_model_11, DBT_y, "DBT-11-feature");
    /////// benchmark ///////

#ifdef ENABLE_OUTPUT_PREDICTION
    // 輸出封包預測與實際搜尋時間至結果檔案
    std::ofstream pt_prediction_out("./INFO/PT_prediction_result.txt");
    if (!pt_prediction_out) {
      std::cerr << "Cannot open PT_prediction_result.txt！" << std::endl;
      return 1;
    }
    std::ofstream kset_prediction_out("./INFO/kset_prediction_result.txt");
    if (!kset_prediction_out) {
      std::cerr << "Cannot open kset_prediction_result.txt！" << std::endl;
      return 1;
    }
    std::ofstream DBT_prediction_out("./INFO/DBT_prediction_result.txt");
    if (!DBT_prediction_out) {
      std::cerr << "Cannot open DBT_prediction_out.txt！" << std::endl;
      return 1;
    }
    for (int i = 0; i < packetNum; ++i) {
      // //  source IP（big-endian 轉 uint32）
      // x_source_ip = ip_to_uint32_be(PT_packets[i].source_ip);
      // //  destination IP（big-endian 轉 uint32）
      // x_destination_ip = ip_to_uint32_be(PT_packets[i].destination_ip);

      ////
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

      // source port（轉 uint16）
      x_source_port = static_cast<double>(PT_packets[i].source_port);
      x_destination_port = static_cast<double>(PT_packets[i].destination_port);
      x_protocol = static_cast<double>(PT_packets[i].protocol);

      double x1_norm_3 = toNormalized(x_source_ip_0, mean_X3[0], std_X3[0]);
      double x2_norm_3 = toNormalized(x_source_ip_1, mean_X3[1], std_X3[1]);
      double x3_norm_3 =
          toNormalized(x_destination_ip_0, mean_X3[2], std_X3[2]);
      double predicted_time_3 =
          predict3(PT_model_3, x1_norm_3, x2_norm_3, x3_norm_3);

      timer.timeReset();
      double x1_norm_11 = toNormalized(x_source_ip_0, mean_X11[0], std_X11[0]);
      double x2_norm_11 = toNormalized(x_source_ip_1, mean_X11[1], std_X11[1]);
      double x3_norm_11 = toNormalized(x_source_ip_2, mean_X11[2], std_X11[2]);
      double x4_norm_11 = toNormalized(x_source_ip_3, mean_X11[3], std_X11[3]);
      double x5_norm_11 =
          toNormalized(x_destination_ip_0, mean_X11[4], std_X11[4]);
      double x6_norm_11 =
          toNormalized(x_destination_ip_1, mean_X11[5], std_X11[5]);
      double x7_norm_11 =
          toNormalized(x_destination_ip_2, mean_X11[6], std_X11[6]);
      double x8_norm_11 =
          toNormalized(x_destination_ip_3, mean_X11[7], std_X11[7]);
      double x9_norm_11 = toNormalized(x_source_port, mean_X11[8], std_X11[8]);
      double x10_norm_11 =
          toNormalized(x_destination_port, mean_X11[9], std_X11[9]);
      double x11_norm_11 = toNormalized(x_protocol, mean_X11[10], std_X11[10]);

      double predicted_time_11 =
          predict11(PT_model_11, x1_norm_11, x2_norm_11, x3_norm_11, x4_norm_11,
                    x5_norm_11, x6_norm_11, x7_norm_11, x8_norm_11, x9_norm_11,
                    x10_norm_11, x11_norm_11);
      _Total_predict_time = timer.elapsed_ns();
      Total_predict_time += _Total_predict_time;
      /*
      double predicted_time_3 = predict3(PT_model_3, x_source_ip_0,
                                         x_source_ip_1, x_destination_ip_0);
      double predicted_time_11 =
          predict11(PT_model_11, x_source_ip_0, x_source_ip_1, x_source_ip_2,
                    x_source_ip_3, x_destination_ip_0, x_destination_ip_1,
                    x_destination_ip_2, x_destination_ip_3, x_source_port,
                    x_destination_port, x_protocol);
      */

      double actual_time = PT_y(i);
      pt_prediction_out << "Packet " << i << "\tPredict3 " << std::fixed
                        << std::setprecision(4) << predicted_time_3
                        << "\tPredict11 " << predicted_time_11
                        << "\tRealTime(ns) " << actual_time << std::endl;
      //
      predicted_time_3 =
          predict3(KSet_model_3, x1_norm_3, x2_norm_3, x3_norm_3);
      predicted_time_11 =
          predict11(KSet_model_11, x1_norm_11, x2_norm_11, x3_norm_11,
                    x4_norm_11, x5_norm_11, x6_norm_11, x7_norm_11, x8_norm_11,
                    x9_norm_11, x10_norm_11, x11_norm_11);
      /*
      predicted_time_3 = predict3(KSet_model_3, x_source_ip_0, x_source_ip_1,
                                  x_destination_ip_0);
      predicted_time_11 =
          predict11(KSet_model_11, x_source_ip_0, x_source_ip_1, x_source_ip_2,
                    x_source_ip_3, x_destination_ip_0, x_destination_ip_1,
                    x_destination_ip_2, x_destination_ip_3, x_source_port,
                    x_destination_port, x_protocol);
      */

      actual_time = KSet_y(i);
      kset_prediction_out << "Packet " << i << "\tPredict3 " << std::fixed
                          << std::setprecision(4) << predicted_time_3
                          << "\tPredict11 " << predicted_time_11
                          << "\tRealTime(ns) " << actual_time << std::endl;
      //
      predicted_time_3 = predict3(DBT_model_3, x1_norm_3, x2_norm_3, x3_norm_3);
      predicted_time_11 =
          predict11(DBT_model_11, x1_norm_11, x2_norm_11, x3_norm_11,
                    x4_norm_11, x5_norm_11, x6_norm_11, x7_norm_11, x8_norm_11,
                    x9_norm_11, x10_norm_11, x11_norm_11);
      /*
      predicted_time_3 = predict3(DBT_model_3, x_source_ip_0, x_source_ip_1,
                                  x_destination_ip_0);
      predicted_time_11 =
          predict11(DBT_model_11, x_source_ip_0, x_source_ip_1, x_source_ip_2,
                    x_source_ip_3, x_destination_ip_0, x_destination_ip_1,
                    x_destination_ip_2, x_destination_ip_3, x_source_port,
                    x_destination_port, x_protocol);
      */

      actual_time = DBT_y(i);
      DBT_prediction_out << "Packet " << i << "\tPredict3 " << std::fixed
                         << std::setprecision(4) << predicted_time_3
                         << "\tPredict11 " << predicted_time_11
                         << "\tRealTime(ns) " << actual_time << std::endl;
    }

    pt_prediction_out.close();
    std::cout << "to pt_prediction_result.txt..." << std::endl;
    kset_prediction_out.close();
    std::cout << "to kset_prediction_result.txt..." << std::endl;
    DBT_prediction_out.close();
    std::cout << "to DBT_prediction_result.txt..." << std::endl;
    std::cout << "AVG predict_time(ns): " << Total_predict_time / packetNum
              << endl;
#endif
  }
#endif
  return 0;
}
