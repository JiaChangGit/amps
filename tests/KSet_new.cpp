#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "KSet.hpp"
#include "input.hpp"
#include "inputFile_test.hpp"
#include "linearSearch_test.hpp"
using namespace std;

#ifndef TIMER_METHOD
#define TIMER_METHOD TIMER_RDTSCP
#endif
#define JIA
// #define PRIVALID
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

int main(int argc, char *argv[]) {
  CommandLineParser parser;
  parser.parseArguments(argc, argv);
  vector<Rule> rule, build, update;
  vector<Packet> packets;
  InputFile inputFile;
  InputFile_test inputFile_test;
  Timer timer;
  unsigned long long time_rdtscp = 0, time_rdtscp2 = 0;
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
  size_t number_update_rule = 0;
  vector<Rule> set_4[4];

  const size_t number_rule = rule.size();
  cout << "The number of rules = " << number_rule << "\n";

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
  cout << ("**************** Construction ****************\n");
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

  if (parser.isSearchMode()) {
    // classification
    cout << ("\n**************** Classification ****************\n");
    const size_t number_pkt = packets.size();
    cout << "\tThe number of packet in the trace file = " << number_pkt << "\n";
    cout << "\tTotal packets run " << trials
         << "times circularly: " << number_pkt * trials << "\n";

    vector<int> matchid(number_pkt);
    Packet p;
#ifdef PRIVALID
    //// === LinearSearch === ////
    int match_miss = 0;
    LinearSearch linearSearch;
//// === LinearSearch === ////
#endif

    if (max_pri_set[1] < max_pri_set[2]) max_pri_set[1] = max_pri_set[2];
#ifdef JIA
    std::ofstream Jlog("perLookTimes.txt", std::ios_base::out);
    if (!Jlog) {
      std::cerr << "Error: Failed to open perLookTimes.txt\n";
      return -1;
    }
#endif
    for (size_t t = 0; t < trials; ++t) {
      for (size_t i = 0; i < number_pkt; ++i) {
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
        time_rdtscp += timer.elapsed_ns();
#ifdef JIA
        if (t == 1) {
          Jlog << "Packet " << i << " \t Result " << matchid[i]
               << " \t Time(ns) " << timer.elapsed_ns() / 1.0 << "\n";
        }
#endif

#ifdef PRIVALID
        //// === LinearSearch === ////
        if (match_pri != (linearSearch.search(rule, p))) {
          ++match_miss;
        }
//// === LinearSearch === ////
#endif
      }
    }
#ifdef JIA
    Jlog.close();
#endif
#ifdef PRIVALID
    cout << "\t" << number_pkt * trials << " packets are classified, "
         << match_miss << " of them are misclassified\n";
#endif
    cout << fixed << setprecision(3)  // 設定小數點後 3 位
         << "\tTotal classification time: " << (time_rdtscp * trials) << " ns"
         << endl
         << "\tAverage classification time: "
         << (time_rdtscp / (trials * number_pkt)) << " ns";
  }

  if (parser.isUpdMode()) {
    cout << ("\n**************** Update ****************\n");
    cout << "\tThe number of updated rules = " << number_update_rule << endl;
    int srcmask, dstmask;

    // insert
    time_rdtscp = 0;
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
      time_rdtscp += timer.elapsed_ns();
    }

    // delete
    time_rdtscp2 = 0;
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
      time_rdtscp2 += timer.elapsed_ns();
    }

    cout << fixed << setprecision(3)  // 設定小數點後 3 位
         << "\tInsert time = " << (time_rdtscp) << " ns, "
         << "delete time = " << (time_rdtscp2) << " ns" << endl
         << "\tTotal update time: " << ((time_rdtscp + time_rdtscp2)) << " ns"
         << endl
         << "\tAverage insert time: " << (time_rdtscp / number_update_rule)
         << " ns, "
         << "Average delete time: " << (time_rdtscp2 / number_update_rule)
         << " ns" << endl
         << "\tAverage update time: "
         << ((time_rdtscp + time_rdtscp2) / (number_update_rule * 2)) << " ns"
         << endl;
  }

  return 0;
}
