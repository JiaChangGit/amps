/**
 * @file main.cpp
 * @brief
 * @author jiachang (jiachanggit@gmail.com)
 * @version 1.0
 * @date 2025-02-20
 *
 * @copyright Copyright (c) 2025  JIA-CHANG
 *
 * @par dialog:
 * <table>
 * <tr><th>Date       <th>Version <th>Author  <th>Description
 * <tr><td>2025-02-20 <td>1.0     <td>jiachang     <td>main
 * </table>
 */

#include "input.hpp"
#include "inputFile_test.hpp"
using namespace std;

#define CHECK
constexpr const char* LoadRule_test_path = "./INFO/loadRule_test.txt";
constexpr const char* LoadPacket_test_path = "./INFO/loadPacket_test.txt";
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
int main(int argc, char* argv[]) {
  CommandLineParser parser;
  parser.parseArguments(argc, argv);

  if (parser.shouldShowHelp()) {
    CommandLineParser::printHelp(argv[0]);
    return 0;
  }
  vector<Rule> ruleV;
  vector<Packet> packetV;
  InputFile inputFile;
  InputFile_test inputFile_test;
  Timer timer;

  inputFile.loadRule(ruleV, parser.getRulesetFile());
  inputFile.loadPacket(packetV, parser.getTraceFile());
  if (parser.isTestMode()) {
    Timer timer;
    timer.timeReset();
    inputFile_test.loadRule_test(ruleV, LoadRule_test_path);
    cout << "Input rule test time(ns): " << timer.elapsed_ns() << "\n";

    timer.timeReset();
    inputFile_test.loadPacket_test(packetV, LoadPacket_test_path);
    cout << "Input packet test time(ns): " << timer.elapsed_ns() << "\n";
  }

  const size_t ruleV_num = ruleV.size();
  ruleV.resize(ruleV_num);

  // ====== //
#ifdef CHECK
  if (ruleV_num <= 0) {
    cerr << "ruleV_num: " << ruleV_num << " <= 0\n";
    exit(1);
  }
#endif
  // ====== //

  cout << "ruleV_num: " << ruleV_num << "\n";
  const size_t packetV_num = packetV.size();

  // ====== //
#ifdef CHECK
  if (packetV_num <= 0) {
    cerr << "packetV_num: " << packetV_num << " <= 0\n";
    exit(1);
  }
#endif
  // ====== //

  cout << "packetV_num: " << packetV_num << "\n";
  packetV.resize(packetV_num);

  return 0;
}
