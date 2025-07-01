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

#ifndef _BASIS_HPP_
#define _BASIS_HPP_

#include <getopt.h>

#include <atomic>
#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>
#include <vector>

// 定義可選的計時模式
#define TIMER_STEADY_CLOCK 1
#define TIMER_RDTSCP 2

// #define TIMER_METHOD TIMER_RDTSCP
// #define TIMER_METHOD TIMER_STEADY_CLOCK
// #ifndef TIMER_METHOD
// #define TIMER_METHOD TIMER_STEADY_CLOCK
// #endif

class Timer {
 private:
#if TIMER_METHOD == TIMER_STEADY_CLOCK
  using Clock = std::chrono::steady_clock;
  using TimePoint = std::chrono::time_point<Clock>;
  TimePoint m_beg;

#elif TIMER_METHOD == TIMER_RDTSCP
  using TimePoint = uint64_t;
  TimePoint m_beg;
  inline static std::atomic<double> cpu_freq_hz{0.0};  // 儲存估算後的 CPU 頻率
#endif

 public:
  Timer() { timeReset(); }

  // 重設起始時間
  __attribute__((always_inline)) inline void timeReset() {
#if TIMER_METHOD == TIMER_STEADY_CLOCK
    m_beg = Clock::now();
#elif TIMER_METHOD == TIMER_RDTSCP
    m_beg = perf_counter();
#endif
  }

  // 回傳秒數
  __attribute__((always_inline)) inline double elapsed_s() const {
#if TIMER_METHOD == TIMER_STEADY_CLOCK
    return std::chrono::duration<double>(Clock::now() - m_beg).count();
#elif TIMER_METHOD == TIMER_RDTSCP
    ensure_cpu_freq_hz();
    return static_cast<double>(perf_counter() - m_beg) / cpu_freq_hz.load();
#endif
  }

  // 回傳奈秒(有無小數)
  __attribute__((always_inline)) inline unsigned long long /*double*/
  elapsed_ns() const {
#if TIMER_METHOD == TIMER_STEADY_CLOCK
    return std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() -
                                                                m_beg)
        .count();
#elif TIMER_METHOD == TIMER_RDTSCP
    ensure_cpu_freq_hz();
    return static_cast<unsigned long long>((perf_counter() - m_beg) *
                                           (1e9 / cpu_freq_hz.load()));
    // return static_cast<double>(perf_counter() - m_beg) *
    //        (1e9 / cpu_freq_hz.load());
#endif
  }

  // 額外提供的 warmup 函式：主動估算 CPU 頻率（若尚未估算）
  static void warmup() {
#if TIMER_METHOD == TIMER_RDTSCP
    ensure_cpu_freq_hz();
#endif
  }

 private:
#if TIMER_METHOD == TIMER_RDTSCP

  static __attribute__((always_inline)) inline uint64_t perf_counter() {
    uint32_t lo, hi;
    __asm__ __volatile__("rdtscp" : "=a"(lo), "=d"(hi)::"%rcx");
    return ((uint64_t)hi << 32) | lo;
  }

  // 實際執行估算邏輯（會被 ensure 呼叫）
  static void init_cpu_freq_hz() {
    constexpr int sleep_us = 10000;  // 10ms
    uint64_t start = perf_counter();
    std::this_thread::sleep_for(std::chrono::microseconds(sleep_us));
    uint64_t end = perf_counter();

    double freq = static_cast<double>(end - start) / (sleep_us * 1e-6);  // Hz
    cpu_freq_hz.store(freq);
    std::cerr << "[Timer] CPU frequency estimated: " << freq * 1e-6 << " MHz\n";
  }

  // 檢查是否已估算過，若無則呼叫初始化
  static void ensure_cpu_freq_hz() {
    if (cpu_freq_hz.load() == 0.0) {
      double expected = 0.0;
      if (cpu_freq_hz.compare_exchange_strong(expected, -1.0)) {
        init_cpu_freq_hz();
      } else {
        // 等待其他執行緒完成初始化
        while (cpu_freq_hz.load() <= 0.0) {
          std::this_thread::yield();
        }
      }
    }
  }

#endif
};

// How to use Timer:
// #if TIMER_METHOD == TIMER_RDTSCP
//   // 手動預熱，避免第一次調用 elapsed_ns() 時的額外延遲
//   Timer::warmup();
// #endif

//   Timer t;
//   // 你的程式邏輯
//   std::cout << "Elapsed: " << t.elapsed_ns() << " ns\n";

// 命令列參數解析類別
class CommandLineParser {
 public:
  void parseArguments(int argc, char* argv[]) {
    int option;
    while ((option = getopt_long(argc, argv, "r:p:beckftsudh", long_options,
                                 nullptr)) != -1) {
      switch (option) {
        case 'r':
          ruleset_file = optarg;
          break;
        case 'p':
          trace_file = optarg;
          break;
        case 'b':
          _BINTH = atoi(optarg);
          break;
        case 'e':
          _END_BOUND = atof(optarg);
          break;
        case 'c':
          _C_BOUND = atoi(optarg);
          break;
        case 'k':
          _TOP_K = atoi(optarg);
          break;
        case 'f': {
          std::vector<int> tmp_in_field;
          int i = 0;
          while (optarg[i] != '\0') {
            if (optarg[i] != ',') {
              char c = optarg[i];
              tmp_in_field.emplace_back(atoi(&c));
            }
            ++i;
          }
          std::cout << "Set pTree field: ";
          for (i = 0; i < tmp_in_field.size() - 1; ++i) {
            std::cout << tmp_in_field[i] << " ";
            _set_field.emplace_back(tmp_in_field[i]);
          }
          _set_field.shrink_to_fit();
          std::cout << "\nSet aTree port field: " << tmp_in_field[i] << "\n";
          _set_port = tmp_in_field[i];
          break;
        }
        case 't':
          test_mode = true;
          break;
        case 's':  // classification simulation
          classificationFlag = true;
          break;
        case 'u':  // update simulation
          updateFlag = true;
          break;
        case 'd':  // prefix dim for DT MT
          _prefix_dims_num = atoi(optarg);
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

  /// @brief PT param -> get_Field(), get_Port()
  /// @return std::vector<uint8_t>, int
  std::vector<uint8_t> get_Field() const { return _set_field; }
  int get_Port() const { return _set_port; }

  /// @brief DBT param -> get_BINTH(), get_C_BOUND(), get_TOP_K(),
  /// get_END_BOUND()
  /// @return int, int, int, double
  int get_BINTH() const { return _BINTH; }
  int get_C_BOUND() const { return _C_BOUND; }
  int get_TOP_K() const { return _TOP_K; }
  double get_END_BOUND() const { return _END_BOUND; }

  /// @brief DT param -> get_PrefixDim(), get_DT_threshold()
  /// @return int, int
  int get_PrefixDim() const { return _prefix_dims_num; }
  int get_DT_threshold() const { return _DT_threshold; }

  static void printHelp(const std::string& program_name) {
    std::cout << "****************************\n";
    std::cout << "Usage: " << program_name << " [options]\n";
    std::cout << "Options:\n";
    std::cout << "  -r, --ruleset <file>  Load ruleset file\n";
    std::cout << "  -p, --trace <file>    Load packet trace file\n";
    std::cout << "  -t, --test            Run test with predefined data\n";
    std::cout << "  -s, --search mode     search mode\n";
    std::cout << "  -u, --update mode     update mode\n";
    std::cout << "  -h, --help            Show this help message\n";
    std::cout << "****************************\n";
  }
  bool load_INFO_param(const char* param_path) {
    std::ifstream infile(param_path);
    if (!infile.is_open()) {
      std::cerr << "Failed to open param: " << param_path << "\n";
      return false;
    }

    std::string line;
    while (std::getline(infile, line)) {
      if (line.empty()) continue;

      std::istringstream iss(line);
      std::string key;
      if (std::getline(iss, key, '=')) {
        std::string val_str;
        if (std::getline(iss, val_str)) {
          std::istringstream val_stream(val_str);

          if (key == "PT_set_field") {
            int val;
            while (val_stream >> val) {
              _set_field.emplace_back(val);  // 全部讀進 vector
            }
          } else if (key == "PT_set_port") {
            val_stream >> _set_port;
          } else if (key == "DBT_binth") {
            val_stream >> _BINTH;
          } else if (key == "DBT_end_bound") {
            val_stream >> _END_BOUND;
          } else if (key == "DBT_top_k") {
            val_stream >> _TOP_K;
          } else if (key == "DBT_c_bound") {
            val_stream >> _C_BOUND;
          } else if (key == "DT_threshold") {
            val_stream >> _DT_threshold;
          } else if (key == "DT_is_prefix_5d") {
            int is_prefix;
            val_stream >> is_prefix;
            _prefix_dims_num = (is_prefix == 1) ? 5 : 2;
          }
        }
      }
    }
    return true;
  }

 private:
  const char* ruleset_file;
  const char* trace_file;
  bool test_mode = false;
  bool show_help = false;
  bool classificationFlag = false;
  bool updateFlag = false;
  static struct option long_options[];
  /// PT ///
  std::vector<uint8_t> _set_field;
  int _set_port = 1;
  /// PT ///
  //// DBT ////
  int _TOP_K = 4;
  double _END_BOUND = 0.8;
  int _C_BOUND = 32;
  int _BINTH = 32;
  //// DBT ////
  //// DT MT ////
  int _prefix_dims_num = 2;
  int _DT_threshold = 7;
  //// DT MT ////
};

/* JIA */ __attribute__((always_inline)) inline double ip_to_uint32_be(
    const unsigned char ip[4]) {
  return static_cast<double>((static_cast<uint32_t>(ip[0]) << 24) |
                             (static_cast<uint32_t>(ip[1]) << 16) |
                             (static_cast<uint32_t>(ip[2]) << 8) |
                             (static_cast<uint32_t>(ip[3])));
}
/* JIA */ __attribute__((always_inline)) inline void extract_ip_bytes_to_float(
    const unsigned char ip[4], float out[4]) {
  for (int i = 0; i < 4; ++i) {
    out[i] = static_cast<float>(ip[i]);
  }
}
// /* JIA */ __attribute__((always_inline)) inline void bytes_reverse(uint32_t
// ip, uint8_t bytes[4]) {
//   bytes[0] = (ip >> 24) & 0xFF;
//   bytes[1] = (ip >> 16) & 0xFF;
//   bytes[2] = (ip >> 8) & 0xFF;
//   bytes[3] = ip & 0xFF;
// }
/* JIA */ __attribute__((always_inline)) inline void bytes_allocate(
    uint32_t ip, uint8_t bytes[4]) {
  bytes[0] = ip & 0xFF;
  bytes[1] = (ip >> 8) & 0xFF;
  bytes[2] = (ip >> 16) & 0xFF;
  bytes[3] = (ip >> 24) & 0xFF;
}
/* JIA */ __attribute__((always_inline)) inline void bytes_allocate_rev(
    const uint8_t bytes[4], uint32_t& ip) {
  ip = static_cast<uint32_t>(bytes[0]) |
       (static_cast<uint32_t>(bytes[1]) << 8) |
       (static_cast<uint32_t>(bytes[2]) << 16) |
       (static_cast<uint32_t>(bytes[3]) << 24);
}
#endif
