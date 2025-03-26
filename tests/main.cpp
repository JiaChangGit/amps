#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "KSet.hpp"
#include "basis.hpp"
#include "input.hpp"
#include "inputFile_test.hpp"
#include "linearSearch_test.hpp"
///////// PT //////////
#include "PT_read.hpp"
#include "pt_tree.hpp"
///////// PT //////////
///////// DBT /////////
#include "DBT_method.hpp"
#include "DBT_read.hpp"
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
// #define PERLOOKUPTIME
#define ENABLE_OUTPUT_PT_PREDICTION
#define ENABLE_OUTPUT_KSET_PREDICTION
#define TOTAL
#ifdef TOTAL
// #include <dlib/mlp.h>

#include <Eigen/Dense>
using namespace Eigen;
// using namespace dlib;
#endif
constexpr const char *LoadRule_test_path = "./INFO/loadRule_test.txt";
constexpr const char *LoadPacket_test_path = "./INFO/loadPacket_test.txt";
// 靜態成員初始化
struct option CommandLineParser::long_options[] = {
    {"ruleset", required_argument, NULL, 'r'},
    {"trace", required_argument, NULL, 'p'},
    {"test", no_argument, NULL, 't'},
    {"classification", no_argument, NULL, 'c'},
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
// ===============================
// Convert Packet -> PT_Packet
// ===============================
std::vector<PT_Packet> convertToPTPackets(const std::vector<Packet> &packets) {
  std::vector<PT_Packet> pt_packets;

  for (const auto &pkt : packets) {
    if (pkt.size() < 5) continue;

    PT_Packet pt{};

    uint32_t sip = pkt[0];
    uint32_t dip = pkt[1];

    // IP 順序修正：big endian
    pt.source_ip[0] = (sip >> 24) & 0xFF;
    pt.source_ip[1] = (sip >> 16) & 0xFF;
    pt.source_ip[2] = (sip >> 8) & 0xFF;
    pt.source_ip[3] = sip & 0xFF;

    pt.destination_ip[0] = (dip >> 24) & 0xFF;
    pt.destination_ip[1] = (dip >> 16) & 0xFF;
    pt.destination_ip[2] = (dip >> 8) & 0xFF;
    pt.destination_ip[3] = dip & 0xFF;

    pt.source_port = static_cast<uint16_t>(pkt[2]);
    pt.destination_port = static_cast<uint16_t>(pkt[3]);
    pt.protocol = pkt[4];

    pt_packets.push_back(pt);
  }

  return pt_packets;
}

// ===============================
// Convert Rule -> PT_Rule
// ===============================
std::vector<PT_Rule> convertToPTRules(const std::vector<Rule> &rules) {
  std::vector<PT_Rule> pt_rules;

  for (const auto &r : rules) {
    if (r.range.size() < 5 || r.prefix_length.size() < 5) continue;

    PT_Rule pt{};
    pt.pri = r.pri;

    // protocol
    pt.protocol[0] = (r.range[4][0] == r.range[4][1]) ? 0xFF : 0x00;
    pt.protocol[1] = static_cast<unsigned char>(r.range[4][0]);

    // source IP
    pt.source_mask = static_cast<unsigned char>(r.prefix_length[0]);
    uint32_t sip = r.range[0][0];
    pt.source_ip[0] = (sip >> 24) & 0xFF;
    pt.source_ip[1] = (sip >> 16) & 0xFF;
    pt.source_ip[2] = (sip >> 8) & 0xFF;
    pt.source_ip[3] = sip & 0xFF;

    // destination IP
    pt.destination_mask = static_cast<unsigned char>(r.prefix_length[1]);
    uint32_t dip = r.range[1][0];
    pt.destination_ip[0] = (dip >> 24) & 0xFF;
    pt.destination_ip[1] = (dip >> 16) & 0xFF;
    pt.destination_ip[2] = (dip >> 8) & 0xFF;
    pt.destination_ip[3] = dip & 0xFF;

    // ports
    pt.source_port[0] = static_cast<uint16_t>(r.range[2][0]);
    pt.source_port[1] = static_cast<uint16_t>(r.range[2][1]);
    pt.destination_port[0] = static_cast<uint16_t>(r.range[3][0]);
    pt.destination_port[1] = static_cast<uint16_t>(r.range[3][1]);

    pt_rules.push_back(pt);
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
    fprintf(fp, "%u.%u.%u.%u\t", pkt.source_ip[0], pkt.source_ip[1],
            pkt.source_ip[2], pkt.source_ip[3]);
    fprintf(fp, "%u.%u.%u.%u\t", pkt.destination_ip[0], pkt.destination_ip[1],
            pkt.destination_ip[2], pkt.destination_ip[3]);
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
    fprintf(fp, "@%u.%u.%u.%u/%u\t", r.source_ip[0], r.source_ip[1],
            r.source_ip[2], r.source_ip[3], r.source_mask);
    fprintf(fp, "%u.%u.%u.%u/%u\t", r.destination_ip[0], r.destination_ip[1],
            r.destination_ip[2], r.destination_ip[3], r.destination_mask);
    fprintf(fp, "%u : %u\t", r.source_port[0], r.source_port[1]);
    fprintf(fp, "%u : %u\t", r.destination_port[0], r.destination_port[1]);
    fprintf(fp, "%02X/%02X\n", r.protocol[1], r.protocol[0]);
  }

  fclose(fp);
}

// 對特徵矩陣 X 做 Z-score 標準化（不包含 bias 欄）
void normalizeFeatures(MatrixXd &X, vector<double> &mean_out,
                       vector<double> &std_out) {
  int rows = X.rows();
  int cols = X.cols();

  mean_out.resize(cols - 1);
  std_out.resize(cols - 1);

  for (int j = 0; j < cols - 1; ++j) {  // 最後一欄是 bias，不處理
    VectorXd col = X.col(j);
    double mean = col.mean();
    double stddev = sqrt((col.array() - mean).square().sum() / rows);

    // 防止除以 0
    if (stddev < 1e-8) stddev = 1.0;

    mean_out[j] = mean;
    std_out[j] = stddev;

    // 執行標準化
    X.col(j) = (X.col(j).array() - mean) / stddev;
  }
}
// 將單一數值做 Z-score 標準化（預測時用）
double toNormalized(double value, double mean, double stddev) {
  if (stddev < 1e-8) return 0.0;
  return (value - mean) / stddev;
}
// 線性迴歸擬合函式
VectorXd linearRegressionFit(const MatrixXd &X, const VectorXd &y) {
  // 使用 Householder QR 分解來求解最小平方解
  return X.householderQr().solve(y);
}
// 模型評估指標
void evaluateModel(const VectorXd &y_pred, const VectorXd &y_true,
                   const string &label) {
  int n = y_true.size();
  double mae = 0.0, mse = 0.0;
  double y_mean = y_true.mean();
  double ss_res = 0.0, ss_tot = 0.0;

  for (int i = 0; i < n; ++i) {
    double error = y_pred(i) - y_true(i);
    mae += std::abs(error);
    mse += error * error;
    ss_res += error * error;
    ss_tot += (y_true(i) - y_mean) * (y_true(i) - y_mean);
  }

  mae /= n;
  mse /= n;
  double rmse = std::sqrt(mse);
  double r2 = 1.0 - (ss_res / ss_tot);

  cout << "\n[" << label << " model evaluation]" << endl;
  cout << "MAE  = " << mae << " ns" << endl;
  cout << "RMSE = " << rmse << " ns" << endl;
  cout << "R^2  = " << r2 << endl;
}
// 參數 a.size() == 4（3個特徵 + 1個 bias）
double predict3(const VectorXd &a, double x1, double x2, double x3) {
  return a(0) * x1 + a(1) * x2 + a(2) * x3 + a(3);
}
// 參數 a.size() == 6（5個特徵 + 1個 bias）
double predict5(const VectorXd &a, double x1, double x2, double x3, double x4,
                double x5) {
  return a(0) * x1 + a(1) * x2 + a(2) * x3 + a(3) * x4 + a(4) * x5 + a(5);
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

  //// PT ////
  auto PT_packets = convertToPTPackets(packets);
  auto PT_rule = convertToPTRules(rule);

  exportPTPackets(PT_packets, "./INFO/pt_packets.txt");
  exportPTRules(PT_rule, "./INFO/pt_rules.txt");

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
  FILE *PT_res_fp = nullptr;
  PT_res_fp = fopen("./INFO/PT_results.txt", "w");
  unsigned long long PT_search_time = 0, _PT_search_time = 0;
  for (int i = 0; i < packetNum; ++i) {
    timer.timeReset();
    PT_match_id = tree.search(PT_packets[i]);
    _PT_search_time = timer.elapsed_ns();
    PT_search_time += _PT_search_time;

    fprintf(PT_res_fp, "Packet %d \t Result %d \t Time(ns) %f\n", i,
            PT_match_id, _PT_search_time / 1.0);
  }
  fclose(PT_res_fp);
  cout << "|- Average search time: " << PT_search_time / packetNum << "ns\n";
  //// PT ////

  //// KSet ////
  vector<Rule> set_4[4];

  const size_t number_rule = rule.size();
  cout << "The number of rules = " << number_rule << "\n";

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
    Packet p;

    if (max_pri_set[1] < max_pri_set[2]) max_pri_set[1] = max_pri_set[2];
#ifdef PERLOOKUPTIME
    std::ofstream KSet_per_log("./INFO/KSet_perLookTimes.txt",
                               std::ios_base::out);
    if (!KSet_per_log) {
      std::cerr << "Error: Failed to open perLookTimes.txt\n";
      return -1;
    }
#endif
    for (size_t t = 0; t < trials; ++t) {
      for (size_t i = 0; i < packetNum; ++i) {
        timer.timeReset();
        p = packets[i];
        int match_pri = -1;
        if (num_set[0] > 0) match_pri = set0.ClassifyAPacket(p);
        if (match_pri < max_pri_set[1] && num_set[1] > 0)
          match_pri = max(match_pri, set1.ClassifyAPacket(p));
        if (match_pri < max_pri_set[2] && num_set[2] > 0)
          match_pri = max(match_pri, set2.ClassifyAPacket(p));
        if (match_pri < max_pri_set[3] && num_set[3] > 0)
          match_pri = max(match_pri, set3.ClassifyAPacket(p));
        matchid[i] = (number_rule - 1) - match_pri;
        _KSet_search_time = timer.elapsed_ns();
        KSet_search_time += _KSet_search_time;
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
    vector<int> matchid(packetNum);
    Packet p;
#ifdef PERLOOKUPTIME
    std::ofstream per_log("./INFO/perLookTimes.txt", std::ios_base::out);
    if (!per_log) {
      std::cerr << "Error: Failed to open perLookTimes.txt\n";
      return -1;
    }
#endif
    ///////// train ////////
    // 三維模型：使用 source_ip, destination_ip, source_port
    MatrixXd X3(packetNum, 4);  // 3 features + bias
    VectorXd PT_model_3(4);
    VectorXd KSet_model_3(4);

    // 五維模型：加上 protocol, destination_port
    MatrixXd X5(packetNum, 6);  // 5 features + bias
    VectorXd PT_model_5(6);
    VectorXd KSet_model_5(6);

    // y 向量共用 (3 and 5)
    VectorXd PT_y(packetNum);
    VectorXd KSet_y(packetNum);

    double x_source_ip = 0, x_destination_ip = 0;
    double x_source_port = 0, x_destination_port = 0, x_protocol = 0;
    double x_source_ip_0 = 0, x_source_ip_1 = 0, x_source_ip_2 = 0,
           x_source_ip_3 = 0;
    double x_destination_ip_0 = 0, x_destination_ip_1 = 0,
           x_destination_ip_2 = 0, x_destination_ip_3 = 0;
    float out[4] = {0};

    for (int i = 0; i < packetNum; ++i) {
      p = packets[i];
      int match_pri = -1;

      // 特徵轉換
      x_protocol = static_cast<double>(PT_packets[i].protocol);
      // x_source_ip = ip_to_uint32_be(PT_packets[i].source_ip);
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
      // x_destination_ip = ip_to_uint32_be(PT_packets[i].destination_ip);
      x_source_port = static_cast<double>(PT_packets[i].source_port);
      x_destination_port = static_cast<double>(PT_packets[i].destination_port);

      // 搜尋時間量測 (PT)
      _PT_search_time = 0;
      timer.timeReset();
      PT_match_id = tree.search(PT_packets[i]);
      _PT_search_time = timer.elapsed_ns();
      PT_y(i) = static_cast<double>(_PT_search_time);

      // 搜尋時間量測 (KSet)
      timer.timeReset();
      if (num_set[0] > 0) match_pri = set0.ClassifyAPacket(p);
      if (match_pri < max_pri_set[1] && num_set[1] > 0)
        match_pri = max(match_pri, set1.ClassifyAPacket(p));
      if (match_pri < max_pri_set[2] && num_set[2] > 0)
        match_pri = max(match_pri, set2.ClassifyAPacket(p));
      if (match_pri < max_pri_set[3] && num_set[3] > 0)
        match_pri = max(match_pri, set3.ClassifyAPacket(p));
      matchid[i] = (number_rule - 1) - match_pri;
      _KSet_search_time = timer.elapsed_ns();
      KSet_y(i) = static_cast<double>(_KSet_search_time);

      // 填入三維特徵：source_ip, destination_ip, source_port + bias
      // X3(i, 0) = x_source_ip;
      // X3(i, 1) = x_destination_ip;
      // X3(i, 2) = x_source_port;
      // X3(i, 3) = 1.0;
      ////
      X3(i, 0) = x_source_ip_0;
      X3(i, 1) = x_source_ip_1;
      X3(i, 2) = x_destination_ip_0;
      X3(i, 3) = 1.0;
      ////

      // 填入五維特徵：source_ip, destination_ip, source_port,
      // destination_port, protocol + bias
      X5(i, 0) = x_source_ip_0;
      X5(i, 1) = x_destination_ip_0;
      X5(i, 2) = x_source_port;
      X5(i, 3) = x_destination_port;
      X5(i, 4) = x_protocol;
      X5(i, 5) = 1.0;
    }
    vector<double> mean_X3, std_X3;
    vector<double> mean_X5, std_X5;

    normalizeFeatures(X3, mean_X3, std_X3);
    normalizeFeatures(X5, mean_X5, std_X5);

    // 模型擬合
    PT_model_3 = linearRegressionFit(X3, PT_y);
    PT_model_5 = linearRegressionFit(X5, PT_y);
    KSet_model_3 = linearRegressionFit(X3, KSet_y);
    KSet_model_5 = linearRegressionFit(X5, KSet_y);

    // 輸出模型參數
    cout << "\n[PT 3-feature model] (x1, x2, x3, bias):\n"
         << PT_model_3.transpose() << endl;
    cout << "\n[PT 5-feature model] (x1~x5, bias):\n"
         << PT_model_5.transpose() << endl;
    cout << "\n[KSet 3-feature model] (x1, x2, x3, bias):\n"
         << KSet_model_3.transpose() << endl;
    cout << "\n[KSet 5-feature model] (x1~x5, bias):\n"
         << KSet_model_5.transpose() << endl;
    ///////// train ////////

    /////// benchmark ///////
    evaluateModel(X3 * PT_model_3, PT_y, "3-feature");
    evaluateModel(X5 * PT_model_5, PT_y, "5-feature");
    evaluateModel(X3 * KSet_model_3, KSet_y, "3-feature");
    evaluateModel(X5 * KSet_model_5, KSet_y, "5-feature");
    /////// benchmark ///////

#ifdef ENABLE_OUTPUT_PT_PREDICTION
    // 輸出封包預測與實際搜尋時間至結果檔案
    std::ofstream pt_prediction_out("./INFO/pt_prediction_result.txt");
    if (!pt_prediction_out) {
      std::cerr << "Cannot open pt_prediction_result.txt！" << std::endl;
      return 1;
    }
    for (int i = 0; i < packetNum; ++i) {
      // // 特徵 2: source IP（big-endian 轉 uint32）
      // x_source_ip = ip_to_uint32_be(PT_packets[i].source_ip);
      // // 特徵 3: destination IP（big-endian 轉 uint32）
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

      // 特徵 4: source port（轉 uint16）
      x_source_port = static_cast<double>(PT_packets[i].source_port);

      double x1_norm_3 = toNormalized(x_source_ip_0, mean_X3[0], std_X3[0]);
      double x2_norm_3 = toNormalized(x_source_ip_1, mean_X3[1], std_X3[1]);
      double x3_norm_3 =
          toNormalized(x_destination_ip_0, mean_X3[2], std_X3[2]);

      double predicted_time_3 =
          predict3(PT_model_3, x1_norm_3, x2_norm_3, x3_norm_3);

      double x1_norm_5 = toNormalized(x_source_ip_0, mean_X5[0], std_X5[0]);
      double x2_norm_5 =
          toNormalized(x_destination_ip_0, mean_X5[1], std_X5[1]);
      double x3_norm_5 = toNormalized(x_source_port, mean_X5[2], std_X5[2]);
      double x4_norm_5 =
          toNormalized(x_destination_port, mean_X5[3], std_X5[3]);
      double x5_norm_5 = toNormalized(x_protocol, mean_X5[4], std_X5[4]);

      double predicted_time_5 = predict5(PT_model_5, x1_norm_5, x2_norm_5,
                                         x3_norm_5, x4_norm_5, x5_norm_5);

      double actual_time = PT_y(i);
      pt_prediction_out << "Packet " << i << "\tPredict3 " << std::fixed
                        << std::setprecision(4) << predicted_time_3
                        << "\tPredict5 " << predicted_time_5
                        << "\tRealTime(ns) " << actual_time << std::endl;
    }

    pt_prediction_out.close();
    std::cout << "to pt_prediction_result.txt..." << std::endl;
#endif

#ifdef ENABLE_OUTPUT_KSET_PREDICTION
    // 輸出封包預測與實際搜尋時間至結果檔案
    std::ofstream kset_prediction_out("./INFO/kset_prediction_result.txt");
    if (!kset_prediction_out) {
      std::cerr << "Cannot open kset_prediction_result.txt！" << std::endl;
      return 1;
    }
    for (int i = 0; i < packetNum; ++i) {
      // // 特徵 2: source IP（big-endian 轉 uint32）
      // x_source_ip = ip_to_uint32_be(PT_packets[i].source_ip);
      // // 特徵 3: destination IP（big-endian 轉 uint32）
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

      // 特徵 4: source port（轉 uint16）
      x_source_port = static_cast<double>(PT_packets[i].source_port);

      double x1_norm_3 = toNormalized(x_source_ip_0, mean_X3[0], std_X3[0]);
      double x2_norm_3 = toNormalized(x_source_ip_1, mean_X3[1], std_X3[1]);
      double x3_norm_3 =
          toNormalized(x_destination_ip_0, mean_X3[2], std_X3[2]);

      double predicted_time_3 =
          predict3(KSet_model_3, x1_norm_3, x2_norm_3, x3_norm_3);

      double x1_norm_5 = toNormalized(x_source_ip_0, mean_X5[0], std_X5[0]);
      double x2_norm_5 =
          toNormalized(x_destination_ip_0, mean_X5[1], std_X5[1]);
      double x3_norm_5 = toNormalized(x_source_port, mean_X5[2], std_X5[2]);
      double x4_norm_5 =
          toNormalized(x_destination_port, mean_X5[3], std_X5[3]);
      double x5_norm_5 = toNormalized(x_protocol, mean_X5[4], std_X5[4]);

      double predicted_time_5 = predict5(KSet_model_5, x1_norm_5, x2_norm_5,
                                         x3_norm_5, x4_norm_5, x5_norm_5);
      double actual_time = KSet_y(i);
      kset_prediction_out << "Packet " << i << "\tPredict3 " << std::fixed
                          << std::setprecision(4) << predicted_time_3
                          << "\tPredict5 " << predicted_time_5
                          << "\tRealTime(ns) " << actual_time << std::endl;
    }

    kset_prediction_out.close();
    std::cout << "to kset_prediction_result.txt..." << std::endl;
#endif
  }
#endif
  return 0;
}
