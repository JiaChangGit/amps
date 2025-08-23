# Adaptive Multi-Classifier Packet System (AMPS)

**An Efficient Packet Classification Framework Using Model-Based Selection Scheme and Reinforcement Learning**
**基於模型導向選擇機制與強化學習的高效封包分類框架**

[![License: MIT](https://img.shields.io/badge/License-MIT-red.svg)](https://opensource.org/licenses/MIT)
[![C++17](https://img.shields.io/badge/C++-17-green.svg)](https://isocpp.org/)
[![CMake](https://img.shields.io/badge/CMake-3.12-green.svg)](https://cmake.org/)
[![Eigen3](https://img.shields.io/badge/Eigen-3-green.svg)](http://eigen.tuxfamily.org/)
[![OpenMP](https://img.shields.io/badge/OpenMP-Required-green.svg)](https://www.openmp.org/)
[![Python](https://img.shields.io/badge/Python-3.10-blue.svg)](https://www.python.org)
[![Torch](https://img.shields.io/badge/Torch-2.7.1-blue.svg)](https://pytorch.org/)
[![Ray](https://img.shields.io/badge/Ray-2.46-blue.svg)](https://www.ray.io/)
[![Gymnasium](https://img.shields.io/badge/Gymnasium-Required-blue.svg)](https://gymnasium.farama.org/)
[![Docker](https://img.shields.io/badge/Docker-Optional-yellow.svg)](https://www.docker.com/)

---

## Overview / 專案概觀

`Adaptive Multi-Classifier Packet System (AMPS)` is a high-performance packet classification framework implemented in C++17 with Eigen3 and Ray RLlib, designed for collaborative multi-classifier systems. It integrates five complementary data structures: **KSet**, **PT-Tree**, **DBTable**, **DynamicTuple**, and **MultilayerTuple**. The framework addresses long-tail latency issues caused by workload skew through two key mechanisms: **query time prediction** and **long-tail-aware filtering**. Additionally, reinforcement learning (RL) optimizes classifier parameters to maximize complementarity, ensuring robust performance across diverse network traffic conditions.

`AMPS` 是一套以 C++17、Eigen3 及 Ray RLlib 實作的高效能封包分類架構，專為多分類器協同運作設計。支援五種互補性資料結構：**KSet**、**PT-Tree**、**DBTable**、**DynamicTuple** 及 **MultilayerTuple**。本系統透過查詢時間預測與長尾封包感知式過濾機制，解決工作負載偏斜導致的長尾延遲問題，並利用強化學習（RL）最佳化分類器參數組合，以在多樣化網路流量下實現穩健效能。

---

## Key Mechanisms / 核心機制

### 1. Linear Regression-Based Latency Estimator (LRLE) / 基於線性回歸的延遲估計器

The framework employs a linear regression model to predict query latency for each classifier (**PT-Tree**, **DBTable**, **DynamicTuple**) based on a packet's five-tuple (source IP, destination IP, source port, destination port, transport protocol). Features are normalized for numerical stability, and models with 3, 5, and 11 input dimensions are tested. Packets are dynamically routed to the classifier with the lowest predicted latency, improving average query performance and mitigating tail latency.

系統採用線性回歸模型，根據封包的五維欄位（來源 IP、目的 IP、來源 port、目的 port、傳輸層協定）預測各分類器（**PT-Tree**、**DBTable**、**DynamicTuple**）的查詢延遲。特徵經正規化以確保數值穩定性，測試了 3、5 及 11 維輸入模型。封包動態路由至預測延遲最小的分類器，提升平均查詢效能並減輕長尾延遲。

### 2. KNN-Based Structure Selection Model (KSSM) / 基於 KNN 的結構選擇模型

A K-Nearest Neighbor (KNN) model (k=3, Manhattan distance) predicts the optimal classifier for each packet using five-tuple features. Labels are derived from baseline measurements, prioritizing the leftmost classifier (PT-Tree > DBTable > DynamicTuple) in case of ties. This lightweight model enhances query efficiency by dynamically selecting the best data structure for varying traffic patterns.

採用 K-近鄰（KNN）模型（k=3，曼哈頓距離），根據五維欄位特徵預測每筆封包的最佳分類器。標籤來自基準測量，平局時優先選擇順序最左的分類器（PT-Tree > DBTable > DynamicTuple）。此輕量級模型通過動態選擇最佳資料結構，提升不同流量模式下的查詢效率。

### 3. Long-Tail-Aware Filtering (LTAF) / 長尾感知式過濾機制

Each classifier identifies slow packets (exceeding the 99th percentile latency threshold) and stores their five-tuple signatures in classifier-specific **Bloom Filters**. Packets matching all filters are routed to the **KSet** classifier for processing, effectively reducing tail latency with minimal overhead.

每個分類器（**PT-Tree**、**DBTable**、**DynamicTuple**、**MultilayerTuple**）識別超過第 99 百分位延遲門檻的慢封包，並將其五維欄位簽名儲存於專屬 **Bloom Filter** 中。若封包同時命中所有過濾器，則交由 **KSet** 分類器處理，以最小開銷降低長尾延遲。

### 4. RL-Based Parameter Optimization / 強化學習參數最佳化

A three-stage reinforcement learning (RL) strategy optimizes classifier parameters to maximize complementarity:

- **First Classifier**: Selects the classifier with the lowest P99 latency.
- **Second Classifier**: Minimizes the intersection of slow packets with the first classifier.
- **Third Classifier**: Further minimizes overlap with the first two classifiers.

Current implementation order: **PT-Tree → DBTable → DynamicTuple**. This approach ensures robust performance under skewed workloads.

採用三階段強化學習（RL）策略最佳化分類器參數，提升互補性：
- **第一分類器**：選擇第 99 百分位延遲最低的分類器。
- **第二分類器**：最小化與第一分類器慢封包的交集。
- **第三分類器**：進一步最小化與前兩分類器的慢封包交集。

目前實作順序為 **PT-Tree → DBTable → DynamicTuple**，確保在偏斜工作負載下實現穩健效能。

---

## Directory Structure / 目錄結構

```
AMPS                                            //
├─ .editorconfig                                //
├─ .mailmap                                     //
├─ CMakeLists.txt                               //
├─ Dockerfile                                   //
├─ INFO                                         //
│  ├─ BloomResults.txt                          //
│  ├─ DBT_IndivResults.txt                      //
│  ├─ DT_IndivResults.txt                       //
│  ├─ KSet_IndivResults.txt                     //
│  ├─ MT_IndivResults.txt                       //
│  ├─ PT_IndivResults.txt                       //
│  ├─ Total_knn_result.txt                      //
│  ├─ Total_model_11_result.txt                 //
│  ├─ Total_model_3_result.txt                  //
│  ├─ Total_model_5_result.txt                  //
│  ├─ Total_prediction_11_result.txt            //
│  ├─ Total_prediction_3_result.txt             //
│  ├─ Total_prediction_5_result.txt             //
│  ├─ Total_prediction_knn_result.txt           //
│  ├─ param.txt                                 //
│  └─ rewards_values.txt                        //
├─ LICENSE                                      //
├─ README.md                                    //
├─ Results                                      //
│  ├─ 0701_171942                               //
│  │  ├─ acl1_100k                              //
│  │  │  ├─ BloomResults.txt                    //
│  │  │  ├─ DBT_IndivResults.txt                //
│  │  │  ├─ DT_IndivResults.txt                 //
│  │  │  ├─ KSet_IndivResults.txt               //
│  │  │  ├─ MT_IndivResults.txt                 //
│  │  │  ├─ PT_IndivResults.txt                 //
│  │  │  ├─ Total_knn_result.txt                //
│  │  │  ├─ Total_model_11_result.txt           //
│  │  │  ├─ Total_model_3_result.txt            //
│  │  │  ├─ Total_model_5_result.txt            //
│  │  │  ├─ Total_prediction_11_result.txt      //
│  │  │  ├─ Total_prediction_3_result.txt       //
│  │  │  ├─ Total_prediction_5_result.txt       //
│  │  │  ├─ Total_prediction_knn_result.txt     //
│  │  │  ├─ main_c_acl1_100k.txt                //
│  │  │  ├─ param.txt                           //
│  │  │  └─ rewards_values.txt                  //
│  │  └─ acl2_100k                              //
│  │     ├─ BloomResults.txt                    //
│  │     ├─ DBT_IndivResults.txt                //
│  │     ├─ DT_IndivResults.txt                 //
│  │     ├─ KSet_IndivResults.txt               //
│  │     ├─ MT_IndivResults.txt                 //
│  │     ├─ PT_IndivResults.txt                 //
│  │     ├─ Total_knn_result.txt                //
│  │     ├─ Total_model_11_result.txt           //
│  │     ├─ Total_model_3_result.txt            //
│  │     ├─ Total_model_5_result.txt            //
│  │     ├─ Total_prediction_11_result.txt      //
│  │     ├─ Total_prediction_3_result.txt       //
│  │     ├─ Total_prediction_5_result.txt       //
│  │     ├─ Total_prediction_knn_result.txt     //
│  │     ├─ main_c_acl2_100k.txt                //
│  │     ├─ param.txt                           //
│  │     └─ rewards_values.txt                  //
├─ build                                        //
│  ├─ main_c_acl1.txt                           //
│  ├─ src                                       //
│  └─ tests                                     //
│     └─ main                                   //
├─ include                                      //
│  ├─ DBT                                       //
│  │  ├─ DBT_core.hpp                           //
│  │  ├─ DBT_data_structure.hpp                 //
│  │  └─ DBT_tss.hpp                            //
│  ├─ DT                                        //
│  │  ├─ dynamictuple-dims.h                    //
│  │  ├─ dynamictuple-ranges.h                  //
│  │  ├─ dynamictuple.h                         //
│  │  ├─ hash.h                                 //
│  │  ├─ hashnode.h                             //
│  │  ├─ hashtable.h                            //
│  │  └─ tuple.h                                //
│  ├─ KNNClassifier.hpp                         //
│  ├─ KSet                                      //
│  │  ├─ KSet.hpp                               //
│  │  ├─ KSet_data_structure.hpp                //
│  │  ├─ input.hpp                              //
│  │  └─ inputFile_test.hpp                     //
│  ├─ LinearRegressionModel.hpp                 //
│  ├─ MT                                        //
│  │  ├─ mhash.h                                //
│  │  ├─ mhashnode.h                            //
│  │  ├─ mhashtable.h                           //
│  │  ├─ mtuple.h                               //
│  │  └─ multilayertuple.h                      //
│  ├─ PT                                        //
│  │  ├─ L3.txt                                 //
│  │  ├─ PT_data_structure.hpp                  //
│  │  └─ PT_tree.hpp                            //
│  ├─ basis.hpp                                 //
│  ├─ bloomFilter.hpp                           //
│  ├─ bobHash.hpp                               //
│  ├─ elementary_DT_MT.h                        //
│  ├─ linearSearch_test.hpp                     //
│  ├─ nanoflann.hpp                             //
│  └─ rl_gym.hpp                                //
├─ scripts                                      //
│  ├─ build.sh                                  //
│  ├─ cache_search_analyz.py                    //
│  ├─ extract_twoColumns.py                     //
│  ├─ part_0                                    //
│  ├─ pcap_to_5tuple.cpp                        //
│  ├─ per_run.sh                                //
│  ├─ plot_CDF.py                               //
│  ├─ run.sh                                    //
│  ├─ run_container.sh                          //
│  ├─ trace_append_rows.cpp                     //
│  └─ trace_statics.py                          //
├─ src                                          //
│  ├─ CMakeLists.txt                            //
│  ├─ DBTable.cpp                               //
│  ├─ DTsrc                                     //
│  │  ├─ dynamictuple-dims.cpp                  //
│  │  ├─ dynamictuple-ranges.cpp                //
│  │  ├─ dynamictuple.cpp                       //
│  │  ├─ hash.cpp                               //
│  │  ├─ hashnode.cpp                           //
│  │  ├─ hashtable.cpp                          //
│  │  └─ tuple.cpp                              //
│  ├─ KSet.cpp                                  //
│  ├─ MTsrc                                     //
│  │  ├─ mhashnode.cpp                          //
│  │  ├─ mhashtable.cpp                         //
│  │  ├─ mtuple.cpp                             //
│  │  └─ multilayertuple.cpp                    //
│  ├─ PT_tree.cpp                               //
│  └─ elementary_DT_MT.cpp                      //
└─ tests                                        //
   ├─ CMakeLists.txt                            //
   ├─ main.cpp                                  //
   ├─ rl_environment.py                         //
   ├─ rl_gym.cpp                                //
   └─ train.py                                  //

```

---

## Build & Run / 編譯與執行

### Prerequisites / 先備套件

Install dependencies on a Linux system (e.g., Ubuntu):

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake libeigen3-dev libomp-dev python3-dev python3-pip git pybind11-dev
pip3 install --upgrade pip
pip3 install pybind11 gymnasium "ray[rllib]" torch numpy
# Verify installations
pip3 list | grep -E "pybind11|gymnasium|ray|torch|numpy"
```

### One-Shot Script / 一鍵執行

Run the following command in the project root directory:

```bash
bash ./scripts/run.sh
# Or
bash ./scripts/per_run.sh
```

- **English**: Auto-detects CPU instruction sets, enables `-march=native -O3` and BMI2/AVX2, builds the project, and runs benchmarks.
- **中文**: 自動偵測 CPU 指令集，啟用 `-march=native -O3` 及 BMI2/AVX2，完成建置並執行基準測試。

> **Note**: The `part_0` trace file is incompatible with `acl5` or `ipc2`. Verify compatibility using `./scripts/run.sh` (`#define VALID # local trace_path="$TRACE_DIR/part_0"`).

> **注意**：`part_0` 追蹤檔案不支援 `acl5` 或 `ipc2`。請使用 `./scripts/run.sh` 確認相容性（`{#define VALID} {# local trace_path="$TRACE_DIR/part_0"}`）。

---

## Performance Evaluation / 效能評估指標

### Metrics / 指標

| Metric            | Definition / 定義                          |
|-------------------|--------------------------------------------|
| **Construction Time** | Time to build classifier (ms) / 建構分類器所需時間 (ms) |
| **Lookup Time**   | Time to search (ns) / 查詢時間 (ns)        |
| **Memory Usage**  | Memory footprint (MB) / 記憶體占用 (MB)    |

- **English**: Results are exported as text reports for analysis.
- **中文**: 測試結果以文字報表形式輸出，方便分析。

---

## References / 參考文獻

- Y.-K. Chang et al., "Efficient Hierarchical Hash-Based Multi-Field Packet Classification With Fast Update for Software Switches," *IEEE Access*, vol. 13, pp. 28962-28978, 2025.
- Z. Liao et al., "PT-Tree: A Cascading Prefix Tuple Tree for Packet Classification in Dynamic Scenarios," *IEEE/ACM Transactions on Networking*, vol. 32, no. 1, pp. 506-519, 2024.
- Z. Liao et al., "DBTable: Leveraging Discriminative Bitsets for High-Performance Packet Classification," *IEEE/ACM Transactions on Networking*, vol. 32, no. 6, pp. 5232-5246, 2024.
- C. Zhang, G. Xie, X. Wang, "DynamicTuple: The Dynamic Adaptive Tuple for High-Performance Packet Classification," *Computer Networks*, vol. 202, 2022.
- C. Zhang, G. Xie, "MultilayerTuple: A General, Scalable and High-Performance Packet Classification Algorithm for SDN," *IFIP Networking*, 2021.
- Blanco, J. L., & Rai, P. K. (2014). *nanoflann: a C++ header-only fork of FLANN, a library for Nearest Neighbor (NN) with KD-trees*. [https://github.com/jlblancoc/nanoflann](https://github.com/jlblancoc/nanoflann).
- Towers, M. et al. (2024). *Gymnasium: A Standard Interface for Reinforcement Learning Environments*. arXiv preprint arXiv:2407.17032.

---

## License / 授權

This project is released under the [MIT License](LICENSE). Contributions via pull requests are welcome!

本專案採用 [MIT 授權](LICENSE)，歡迎透過 Pull Request 參與開發！

---

## Contact / 聯繫

For questions or contributions, please open an issue or submit a pull request on [GitHub](https://github.com/JiaChangGit/multiple_parallel_PC).

如有問題或欲貢獻程式碼，請在 [GitHub](https://github.com/JiaChangGit/multiple_parallel_PC) 上開啟 Issue 或提交 Pull Request。
