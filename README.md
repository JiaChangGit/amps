# multiple_parallel_PC

A High-Performance, Multi-Algorithm Packet Classification Framework
高效能多演算法封包分類框架

---

## Overview / 專案概觀

**English**
`multiple_parallel_PC` is a high-performance multi-algorithm packet classification framework written in C++17. It dynamically determines whether each packet should be processed by one of the KSet, PT-Tree, DBTable, DynamicTuple, or MultilayerTuple algorithms by using a linear regression model or a Bloom Filter. This mechanism balances the query latency and facilitates in-depth comparison of the performance trade-offs among algorithms.

**中文**
`multiple_parallel_PC` 是一套以 C++17 撰寫的高效能多演算法封包分類框架。它透過線性回歸模型或 Bloom Filter 快速檢測，動態判斷每一筆封包應交由 KSet、PT-Tree、DBTable、DynamicTuple 或 MultilayerTuple 其中一種演算法處理。此機制可平衡查詢延遲，便於深入比較各演算法的效能取捨。

---

## Features / 特色

| Feature / 特色             | Description / 說明                                                                                      |
|----------------------------|--------------------------------------------------------------------------------------------------------|
| **Adaptive Dispatch**      | Linear regression or Bloom filter predicts the most efficient classifier for each packet in ~10 ns.    |
| **Multi-Classifier Core**  | Built-in support for KSet, PT-Tree, DBTable, DynamicTuple, MultilayerTuple.                            |
| **SIMD / PEXT Optimization** | AVX2/SSE2 intrinsics, BMI2 PEXT, and cache-aligned data structures for maximum throughput.           |
| **Transparent Metrics**    | Construction time, throughput (pps), and memory footprint are measured automatically.                  |
| **Scripted Benchmarking**  | `scripts/run.sh` builds, runs, and reports on synthetic or trace-driven workloads.                     |

---

## Directory Structure / 目錄結構

```
multiple_parallel_PC                     //
├─ .editorconfig                         //
├─ .mailmap                              //
├─ CMakeLists.txt                        //
├─ INFO                                  //
│  ├─ BloomResults.txt                   //
│  ├─ DBT_IndivResults.txt               //
│  ├─ DBT_buckets.txt                    //
│  ├─ DBT_nodes.txt                      //
│  ├─ DBT_prediction_11_result.txt       //
│  ├─ DBT_prediction_3_result.txt        //
│  ├─ DBT_prediction_5_result.txt        //
│  ├─ DT_IndivResults.txt                //
│  ├─ DT_prediction_11_result.txt        //
│  ├─ DT_prediction_3_result.txt         //
│  ├─ DT_prediction_5_result.txt         //
│  ├─ KSet_IndivResults.txt              //
│  ├─ KSet_prediction_11_result.txt      //
│  ├─ KSet_prediction_3_result.txt       //
│  ├─ KSet_prediction_5_result.txt       //
│  ├─ MT_IndivResults.txt                //
│  ├─ MT_prediction_11_result.txt        //
│  ├─ MT_prediction_3_result.txt         //
│  ├─ MT_prediction_5_result.txt         //
│  ├─ PT_IndivResults.txt                //
│  ├─ PT_prediction_11_result.txt        //
│  ├─ PT_prediction_3_result.txt         //
│  ├─ PT_prediction_5_result.txt         //
│  ├─ Total_model_11_result.txt          //
│  ├─ Total_model_3_result.txt           //
│  ├─ Total_model_5_result.txt           //
│  ├─ Total_prediction_11_result.txt     //
│  ├─ Total_prediction_3_result.txt      //
│  └─ Total_prediction_5_result.txt      //
├─ LICENSE                               //
├─ README.md                             //
├─ build                                 //
│  ├─ main_c_acl1.txt                    //
│  ├─ main_c_acl2.txt                    //
│  ├─ main_c_acl3.txt                    //
│  ├─ main_c_acl4.txt                    //
│  ├─ main_c_acl5.txt                    //
│  ├─ main_c_fw1.txt                     //
│  ├─ main_c_fw2.txt                     //
│  ├─ main_c_fw3.txt                     //
│  ├─ main_c_fw4.txt                     //
│  ├─ main_c_fw5.txt                     //
│  ├─ main_c_ipc1.txt                    //
│  ├─ main_c_ipc2.txt                    //
│  ├─ src                                //
│  └─ tests                              //
│     └─ main                            //
├─ include                               //
│  ├─ DBT                                //
│  │  ├─ DBT_core.hpp                    //
│  │  ├─ DBT_data_structure.hpp          //
│  │  ├─ DBT_read.hpp                    //
│  │  └─ DBT_tss.hpp                     //
│  ├─ DT                                 //
│  │  ├─ dynamictuple-dims.h             //
│  │  ├─ dynamictuple-ranges.h           //
│  │  ├─ dynamictuple.h                  //
│  │  ├─ hash.h                          //
│  │  ├─ hashnode.h                      //
│  │  ├─ hashtable.h                     //
│  │  └─ tuple.h                         //
│  ├─ KSet                               //
│  │  ├─ KSet.hpp                        //
│  │  ├─ KSet_data_structure.hpp         //
│  │  ├─ input.hpp                       //
│  │  └─ inputFile_test.hpp              //
│  ├─ LinearRegressionModel.hpp          //
│  ├─ MT                                 //
│  │  ├─ mhash.h                         //
│  │  ├─ mhashnode.h                     //
│  │  ├─ mhashtable.h                    //
│  │  ├─ mtuple.h                        //
│  │  └─ multilayertuple.h               //
│  ├─ PT                                 //
│  │  ├─ L3.txt                          //
│  │  ├─ PT_data_structure.hpp           //
│  │  ├─ PT_read.hpp                     //
│  │  └─ PT_tree.hpp                     //
│  ├─ basis.hpp                          //
│  ├─ bloomFilter.hpp                    //
│  ├─ bobHash.hpp                        //
│  ├─ elementary_DT_MT.h                 //
│  ├─ io_DT_MT.h                         //
│  ├─ linearSearch_test.hpp              //
│  ├─ part_0                             //
│  └─ trie_DT_MT.h                       //
├─ scripts                               //
│  ├─ append_rows.cpp                    //
│  ├─ build.sh                           //
│  ├─ extract_from_txt.py                //
│  ├─ pcap_to_5tuple.cpp                 //
│  ├─ per_run.sh                         //
│  ├─ run.sh                             //
│  └─ static.py                          //
├─ src                                   //
│  ├─ CMakeLists.txt                     //
│  ├─ DBTable.cpp                        //
│  ├─ DTsrc                              //
│  │  ├─ dynamictuple-dims.cpp           //
│  │  ├─ dynamictuple-ranges.cpp         //
│  │  ├─ dynamictuple.cpp                //
│  │  ├─ hash.cpp                        //
│  │  ├─ hashnode.cpp                    //
│  │  ├─ hashtable.cpp                   //
│  │  └─ tuple.cpp                       //
│  ├─ KSet.cpp                           //
│  ├─ MTsrc                              //
│  │  ├─ mhashnode.cpp                   //
│  │  ├─ mhashtable.cpp                  //
│  │  ├─ mtuple.cpp                      //
│  │  └─ multilayertuple.cpp             //
│  ├─ PT_tree.cpp                        //
│  └─ elementary_DT_MT.cpp               //
└─ tests                                 //
   ├─ CMakeLists.txt                     //
   └─ main.cpp                           //

../classbench_set/ipv4-ruleset/acl1_100k     //
../classbench_set/ipv4-trace/acl1_100k_trace //
......

```

---

## Build & Run / 編譯與執行

### Prerequisites / 先備套件

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake libeigen3-dev libomp-dev
```

### One-Shot Script / 一鍵執行

```bash
# In the project root directory
sudo bash ./scripts/run.sh
```

This script auto-detects CPU instruction sets, enables `-march=native -O3 -flto` and BMI2/AVX2 options, builds the project, runs default benchmarks, and outputs results.

此腳本會自動偵測 CPU 指令集，啟用 `-march=native -O3 -flto` 及 BMI2/AVX2 選項，完成建置後立即執行預設基準測試並輸出統計結果。

---

## Performance Evaluation / 效能評估指標

| Metric / 指標             | Definition / 定義                        |
|--------------------------|------------------------------------------|
| **Construction Time**    | Time to build classifier (ms) / 建構分類器所需時間 (ms) |
| **Lookup Throughput**    | Query rate (Mpps) / 查詢吞吐量 (Mpps)    |
| **Memory Usage**         | Memory footprint (MB) / 記憶體占用 (MB)  |

Benchmark results are exported as txt reports for further analysis.
測試結果將以 txt 輸出，方便後續分析。

---

## References / 參考文獻

1. Y.-K. Chang et al. "Efficient Hierarchical Hash-Based Multi-Field Packet Classification With Fast Update for Software Switches," *IEEE Access*, vol. 13, pp. 28962‑28978, 2025.
2. Z. Liao et al. "PT‑Tree: A Cascading Prefix Tuple Tree for Packet Classification in Dynamic Scenarios," *IEEE/ACM Transactions on Networking*, vol. 32, no. 1, pp. 506‑519, 2024.
3. Z. Liao et al. "DBTable: Leveraging Discriminative Bitsets for High‑Performance Packet Classification," *IEEE/ACM Transactions on Networking*, vol. 32, no. 6, pp. 5232‑5246, 2024.
4. C. Zhang, G. Xie, X. Wang. "DynamicTuple: The Dynamic Adaptive Tuple for High‑Performance Packet Classification," *Computer Networks*, vol. 202, 2022.
5. C. Zhang, G. Xie. "MultilayerTuple: A General, Scalable and High‑Performance Packet Classification Algorithm for SDN," *IFIP Networking*, 2021.

---

## License / 授權

Released under the **MIT License**. Contributions via pull requests are welcome!

本專案採用 **MIT 授權**，歡迎提交 Pull Request 共同參與開發！

---

> For more details, please refer to the source code and scripts in this repository.
> 詳細內容請參考本專案原始碼與腳本。

