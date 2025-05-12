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

## Result / 結果

main_acl2.txt:

```

The number of rules = 89229
The number of packets = 892290

PT search config time: 2.41164 s
0 4 2
**************** Construction(PT) ****************

Start build for single thread...
|- Using fields:     0,4,2,1
|- Construct time:   5330170ns
|- tree.totalNodes: 23734
|- Memory footprint: 10.8329MB
binth=4 th_b=0.8 K=4 th_c=32

**************** Construction(DBT) ****************
buckets        : 24283
max bucket size: 836
target buckets : 19759 81.37%
(10,50]        : 1470 6.05%
(50,100]       : 120 0.49%
big cell       : 30 0.12%

DBS bitsNum: 17


rule_num 89229 89229
in_bucket 28861 0.32
in_tuple 60368 0.68
total buckets  : 189199
used buckets   : 41835 22.11%
max bucket size: 42
target buckets : 36226 86.59%
(10,50]        : 848 2.03%
(50,100]       : 0 0.00%
big cell       : 0 0.00%
tuple spaces   : 131
avg tuples     : 3.48
max tuples     : 17

Construction Time: 2366973734 ns

Total memory 12.14 MB

**************** Construction(KSet) ****************
================KSet Precompute=============
Set 0: 61768, Set 1: 13149, Set 2: 13174, Set 3: 1138
max_pri[0]: 89228, max_pri[1]: 70780, max_pri[2]: 70961, max_pri[3]: 12092
Set 0: 15 bits, Set 1: 13 bits, Set 2: 13 bits
================Compute=============
Set 0: 66377, Set 1: 14771, Set 2: 7339, Set 3: 742
max_pri[0]: 89228, max_pri[1]: 68568, max_pri[2]: 67749, max_pri[3]: 8467
non-empty_seg[0] = 0, non-empty_seg[1] = 0, non-empty_seg[2] = 0
AVG[0]: inf, AVG[1]: inf, AVG[2]: inf
MAX[0]: 32623, MAX[1]: 32623, MAX[2]: 32623
	Construction time: 49750096 ns

	***Set 0:***
	rules in set: 66377, rules in small node: 13470, rules in big node: 52907
	tablesize: 32768, NULL_Node_Count = 26157, Small_Node_Count = 5407, Big_Node_Count = 1204
	Table_memory: 128.000(KB), Total_Rules_memory(Small): 368.320(KB), Total_Rules_memory(Big): 1581.660(KB)
	Total memory: 2077.980(KB), Byte/rule: 32.057

	***Set 1:***
	rules in set: 14771, rules in small node: 1764, rules in big node: 13007
	tablesize: 8192, NULL_Node_Count = 7382, Small_Node_Count = 467, Big_Node_Count = 343
	Table_memory: 32.000(KB), Total_Rules_memory(Small): 48.234(KB), Total_Rules_memory(Big): 380.414(KB)
	Total memory: 460.648(KB), Byte/rule: 31.934

	***Set 2:***
	rules in set: 7339, rules in small node: 506, rules in big node: 6833
	tablesize: 8192, NULL_Node_Count = 7834, Small_Node_Count = 223, Big_Node_Count = 135
	Table_memory: 32.000(KB), Total_Rules_memory(Small): 13.836(KB), Total_Rules_memory(Big): 199.281(KB)
	Total memory: 245.117(KB), Byte/rule: 34.201

	***Set 3:***
	rules in set: 742, rules in small node: 0, rules in big node: 742
	tablesize: 1, NULL_Node_Count = 0, Small_Node_Count = 0, Big_Node_Count = 1
	Table_memory: 0.004(KB), Total_Rules_memory(Small): 0.000(KB), Total_Rules_memory(Big): 22.551(KB)
	Total memory: 22.555(KB), Byte/rule: 31.127
**************** Construction(DT) ****************
	Construction time: 3448138055 ns
DT_data_memory_size: 4.425, DT_index_memory_size: 7.906
Total: 12.331
DT tuples_num: 18
**************** Construction(MT) ****************
	Construction time: 20314181 ns
MT_data_memory_size: 4.425, MT_index_memory_size: 8.557
Total: 12.982
MT tuples_num: 9

**************** Build(Model 3-D and 11-D) ****************
	The number of packet in the trace file = 892290
	Total packets run 4 times circularly: 3569160

========= Model parameters =========

[PT 3-feature model] (x1, x2, x3):
 0.054 -0.090  0.006

[PT 5-feature model] (x1~x5):
 0.063 -0.100 -0.048  0.108  0.054

[PT 11-feature model] (x1~x11):
 0.069 -0.103  0.005 -0.028 -0.043  0.105  0.053 -0.004  0.069 -0.045 -0.021

[DBT 3-feature model] (x1, x2, x3):
-0.016  0.028 -0.024

[DBT 5-feature model] (x1~x5):
-0.021  0.033  0.001 -0.035 -0.055

[DBT 11-feature model] (x1~x11):
-0.015  0.049 -0.008 -0.015  0.007 -0.035 -0.040 -0.022  0.068 -0.073  0.213

[KSet 3-feature model] (x1, x2, x3):
-3.647 -1.683  2.931

[KSet 5-feature model] (x1~x5):
-3.642 -1.794  2.182  2.750 -1.719

[KSet 11-feature model] (x1~x11):
-2.852 -0.397 -0.289  1.487  1.119  3.003 -1.935  2.669  3.341 14.474  8.921

[DT 3-feature model] (x1, x2, x3):
 0.067  0.016 -0.109

[DT 5-feature model] (x1~x5):
 0.080  0.005 -0.160  0.063  0.132

[DT 11-feature model] (x1~x11):
 0.075  0.012 -0.049  0.092 -0.161  0.058  0.132  0.042 -0.008 -0.062  0.014

[MT 3-feature model] (x1, x2, x3):
-0.005 -1.292  1.095

[MT 5-feature model] (x1~x5):
-0.083 -1.292  0.965  1.244 -1.831

[MT 11-feature model] (x1~x11):
 0.472  0.030 -1.358 -0.841  0.556  1.344 -1.787  2.445  3.153  6.831  9.996

**************** Model(Acc and Fail) ****************
    model_acc 3 (%): 24.286
    model_fail 3 (%): 1.235
    model_oth 3 (%): 74.479
    model_acc 5 (%): 24.962
    model_fail 5 (%): 1.500
    model_oth 5 (%): 73.538
    model_acc 11 (%): 38.518
    model_fail 11 (%): 1.045
    model_oth 11 (%): 60.437

**************** Classification(Model) ****************

|=== AVG predict time(Model-3): 10ns
|=== AVG search with predict time(Model-3): 32ns
|=== PT, DBT, KSET, DT, MT (%): 12.553, 77.442, 0.275, 9.393, 0.337
|--- Total_y 25th Percentile: 20.000
|--- Total_y median Percentile: 20.000
|--- Total_y 75th Percentile: 21.000
|--- Total_y 95th Percentile: 30.000
|--- Total_y 99th Percentile: 31.000
|--- Total_y StdDev: 58.456 (dispersity)

|=== AVG predict time(Model-5): 6ns
|=== AVG search with predict time(Model-5): 28ns
|=== PT, DBT, KSET, DT, MT (%): 16.092, 63.700, 0.360, 18.535, 1.314
|--- Total_y 25th Percentile: 20.000
|--- Total_y median Percentile: 20.000
|--- Total_y 75th Percentile: 21.000
|--- Total_y 95th Percentile: 30.000
|--- Total_y 99th Percentile: 31.000
|--- Total_y StdDev: 66.287 (dispersity)

|=== AVG predict time(Model-11): 5ns
|=== AVG search with predict time(Model-11): 27ns
|=== PT, DBT, KSET, DT, MT (%): 41.532, 27.937, 0.102, 30.138, 0.292
|--- Total_y 25th Percentile: 20.000
|--- Total_y median Percentile: 20.000
|--- Total_y 75th Percentile: 21.000
|--- Total_y 95th Percentile: 30.000
|--- Total_y 99th Percentile: 31.000
|--- Total_y StdDev: 38.630 (dispersity)

**************** Classification(BLOOM) ****************

|=== AVG predict time(BloomFilter): 15ns
|=== AVG search time with predict(BloomFilter): 50ns
|=== PT, DBT, KSET, DT, MT (%): 6.721, 58.247, 6.165, 1.952, 26.666
|--- BloomFilter_y 25th Percentile: 20.000
|--- BloomFilter_y median Percentile: 20.000
|--- BloomFilter_y 75th Percentile: 31.000
|--- BloomFilter_y 95th Percentile: 61.000
|--- BloomFilter_y 99th Percentile: 160.000
|--- BloomFilter_y StdDev: 107.909 (dispersity)

************** Classification(Individual) **************
**************** Classification(PT) ****************
|--- T_PT_y 25th Percentile: 50.000
|--- T_PT_y median Percentile: 110.000
|--- T_PT_y 75th Percentile: 201.000
|--- T_PT_y 95th Percentile: 382.000
|--- T_PT_y 99th Percentile: 613.000
|--- T_PT_y StdDev: 193.805 (dispersity)
|- Average search time: 157ns

**************** Classification(DBT) ****************
|--- T_DBT_y 25th Percentile: 41.000
|--- T_DBT_y median Percentile: 90.000
|--- T_DBT_y 75th Percentile: 201.000
|--- T_DBT_y 95th Percentile: 332.000
|--- T_DBT_y 99th Percentile: 472.000
|--- T_DBT_y StdDev: 227.118 (dispersity)
|- Average search time: 139ns

avg_acc_bucket: 1.563 max: 2
avg_acc_tuple: 5.047 max: 22
avg_acc_rule: 7.181 max: 70

**************** Classification(KSet) ****************
|--- T_KSet_y 25th Percentile: 60.000
|--- T_KSet_y median Percentile: 201.000
|--- T_KSet_y 75th Percentile: 633.000
|--- T_KSet_y 95th Percentile: 1096.000
|--- T_KSet_y 99th Percentile: 1487.000
|--- T_KSet_y StdDev: 548.138 (dispersity)
	Average search time: 372 ns

**************** Classification(DT) ****************
|--- T_DT_y 25th Percentile: 40.000
|--- T_DT_y median Percentile: 81.000
|--- T_DT_y 75th Percentile: 201.000
|--- T_DT_y 95th Percentile: 392.000
|--- T_DT_y 99th Percentile: 603.000
|--- T_DT_y StdDev: 222.486 (dispersity)
	Average search time: 140 ns

**************** Classification(MT) ****************
|--- T_MT_y 25th Percentile: 49.000
|--- T_MT_y median Percentile: 99.000
|--- T_MT_y 75th Percentile: 307.000
|--- T_MT_y 95th Percentile: 585.000
|--- T_MT_y 99th Percentile: 972.000
|--- T_MT_y StdDev: 299.510 (dispersity)
	Average search time: 200 ns

```

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

