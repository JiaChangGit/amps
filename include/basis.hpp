/**
 * @file basis.hpp
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
 * <tr><td>2025-02-20 <td>1.0     <td>jiachang     <td>basic data structures
 * </table>
 */

#ifndef __LIB_BASIS_HPP__
#define __LIB_BASIS_HPP__

#include <getopt.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <iomanip>  // 用於設置小數點位數
#include <iostream>
#include <string>
#include <vector>
#define LowDim 0
#define HighDim 1
#define MAXDIMENSIONS 5

typedef std::vector<uint32_t> Packet;

struct Rule {
  Rule(unsigned int dim = 5)
      : dim(dim),
        pri(0),
        priority(0),
        range(dim, {{0, 0}}),
        prefix_length(dim, 0) {};

  unsigned int dim;
  int pri, priority;
  std::vector<std::array<uint32_t, 2>> range;
  std::vector<uint32_t> prefix_length;

  bool inline isMatch(const Packet& p) const {
    for (size_t i = 0; i < dim; ++i) {
      if (p[i] < range[i][LowDim] || p[i] > range[i][HighDim]) return false;
    }
    return true;
  }
};

class PacketClassifier {
 public:
  virtual void ConstructClassifier(const std::vector<Rule>& rules) = 0;
  virtual int ClassifyAPacket(const Packet& packet) = 0;
  virtual void DeleteRule(const Rule& rule) = 0;
  virtual void InsertRule(const Rule& rule) = 0;

 private:
};

// 定義可選的計時模式
#define TIMER_STEADY_CLOCK 1
#define TIMER_RDTSCP 2

// 讓 main.cpp 選擇計時模式
#ifndef TIMER_METHOD
#define TIMER_METHOD TIMER_STEADY_CLOCK  // 預設使用 std::chrono
#endif

class Timer {
 private:
#if TIMER_METHOD == TIMER_STEADY_CLOCK
  using Clock = std::chrono::steady_clock;
  using TimePoint = std::chrono::time_point<Clock>;

  TimePoint m_beg;
#elif TIMER_METHOD == TIMER_RDTSCP
  using TimePoint = uint64_t;
  TimePoint m_beg;
#endif

 public:
  Timer() { timeReset(); }

  inline void timeReset() {
#if TIMER_METHOD == TIMER_STEADY_CLOCK
    m_beg = Clock::now();
#elif TIMER_METHOD == TIMER_RDTSCP
    m_beg = perf_counter();
#endif
  }

  inline double elapsed_s() const {
#if TIMER_METHOD == TIMER_STEADY_CLOCK
    return std::chrono::duration<double>(Clock::now() - m_beg).count();
#elif TIMER_METHOD == TIMER_RDTSCP
    return static_cast<double>(perf_counter() - m_beg) / get_cpu_frequency_hz();
#endif
  }

  inline unsigned long long elapsed_ns() const {
#if TIMER_METHOD == TIMER_STEADY_CLOCK
    return std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() -
                                                                m_beg)
        .count();
#elif TIMER_METHOD == TIMER_RDTSCP
    return static_cast<unsigned long long>((perf_counter() - m_beg) *
                                           (1e9 / get_cpu_frequency_hz()));
#endif
  }

 private:
#if TIMER_METHOD == TIMER_RDTSCP
  inline uint64_t perf_counter() const {
    uint32_t lo, hi;
    __asm__ __volatile__("rdtscp" : "=a"(lo), "=d"(hi));
    return ((uint64_t)lo) | (((uint64_t)hi) << 32);
  }

  inline double get_cpu_frequency_hz() const {
    return 5.0e9;  // 假設 CPU 時脈為 5 GHz，需根據實際 CPU 頻率調整
  }
#endif
};
// How to use Timer:
// #ifndef TIMER_METHOD
// #define TIMER_METHOD TIMER_RDTSCP
// #endif
// 或 TIMER_STEADY_CLOCK
// #include "Timer.h"
// Timer t;
// t.timeReset();
// // 執行你的函數
// double timeTaken = t.elapsed_s();
// std::cout << "Time taken: " << timeTaken << " seconds\n";

// 命令列參數解析類別
class CommandLineParser {
 public:
  void parseArguments(int argc, char* argv[]) {
    int option;
    while ((option = getopt_long(argc, argv, "r:p:tcuh", long_options,
                                 nullptr)) != -1) {
      switch (option) {
        case 'r':
          ruleset_file = optarg;
          break;
        case 'p':
          trace_file = optarg;
          break;
        case 't':
          test_mode = true;
          break;
        case 'c':  // classification simulation
          classificationFlag = true;
          break;
        case 'u':  // update simulation
          updateFlag = true;
          break;
        case 'h':
          show_help = true;
          break;
        case '?':
        default:
          std::cerr << "Invalid option or missing argument.\n";
          show_help = true;
      }
    }
  }

  const char* getRulesetFile() const { return ruleset_file; }
  const char* getTraceFile() const { return trace_file; }
  bool isTestMode() const { return test_mode; }
  bool isSearchMode() const { return classificationFlag; }
  bool isUpdMode() const { return updateFlag; }
  bool shouldShowHelp() const { return show_help; }

  static void printHelp(const std::string& program_name) {
    std::cout << "****************************\n";
    std::cout << "Usage: " << program_name << " [options]\n";
    std::cout << "Options:\n";
    std::cout << "  -r, --ruleset <file>  Load ruleset file\n";
    std::cout << "  -p, --trace <file>    Load packet trace file\n";
    std::cout << "  -t, --test            Run test with predefined data\n";
    std::cout << "  -c, --search mode     search mode\n";
    std::cout << "  -u, --update mode     update mode\n";
    std::cout << "  -h, --help            Show this help message\n";
    std::cout << "****************************\n";
  }

 private:
  const char* ruleset_file;
  const char* trace_file;
  bool test_mode = false;
  bool show_help = false;
  bool classificationFlag = false;
  bool updateFlag = false;
  static struct option long_options[];
};
#endif
