# multiple_parallel_PC

**A High-Performance, Multi-Classifier Packet Classification Framework with Latency Prediction and Complementary Optimization**
**高效能多分類器封包分類架構，結合延遲預測與互補性最佳化**

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![C++17](https://img.shields.io/badge/C++-17-blue.svg)](https://isocpp.org/)
[![Eigen3](https://img.shields.io/badge/Eigen-3-green.svg)](http://eigen.tuxfamily.org/)

---

## Overview / 專案概觀

`multiple_parallel_PC` is a high-performance packet classification framework implemented in C++17 with Eigen3, designed for collaborative multi-classifier systems. It integrates five heterogeneous and complementary packet classification data structures: **KSet**, **PT-Tree**, **DBTable**, **DynamicTuple**, and **MultilayerTuple**. The framework leverages two independent mechanisms—latency-aware classifier selection and long-tail packet filtering—to optimize performance under diverse network traffic conditions. A reinforcement learning (RL)-based strategy further enhances classifier complementarity through parameter optimization.

`multiple_parallel_PC` 是一套以 C++17 與 Eigen3 實作的高效能封包分類架構，專為多分類器協同運作設計。支援五種具異質性與互補性的資料結構：**KSet**、**PT-Tree**、**DBTable**、**DynamicTuple** 及 **MultilayerTuple**。透過延遲預測導向的分類器選擇、長尾封包感知式過濾機制，以及強化學習（RL）參數最佳化策略，系統能在多樣化的網路流量下實現高效能。

---

## Key Mechanisms / 核心機制

### 1. Latency-Aware Classifier Prediction / 延遲預測導向的分類器選擇

The framework employs a multidimensional linear regression model to predict query latency for each classifier (**PT-Tree**, **DBTable**, **DynamicTuple**) based on a packet's five-tuple (source IP, destination IP, source port, destination port, transport protocol). It dynamically selects the classifier with the lowest predicted latency, improving average query performance and mitigating degradation under high-variability traffic.

系統利用多維線性回歸模型，根據封包的五維欄位（來源 IP、目的 IP、來源 port、目的 port、傳輸層協定），預測各分類器（**PT-Tree**、**DBTable**、**DynamicTuple**）的查詢延遲，並動態選擇延遲最小的分類器。此機制提升平均查詢效能，避免高變異度流量下的效能劣化。

### 2. Long-Tail-Aware Filtering / 長尾封包感知式過濾機制

To address high-latency tail packets, each classifier identifies packets exceeding the 99th percentile latency threshold as "slow packets". Their five-tuple signatures are stored in a **Bloom Filter**, **Cuckoo Filter**, or **Elastic Bloom Filter**. Packets matching the long-tail filters of all classifiers are routed to the stable **KSet** classifier, reducing tail latency effectively.

為處理高延遲的長尾封包，每個分類器（**PT-Tree**、**DBTable**、**DynamicTuple**、**MultilayerTuple**）將超過第 99 百分位延遲門檻的封包標記為「慢封包」，並將其五維欄位簽名儲存於 **Bloom Filter**、**Cuckoo Filter** 或 **Elastic Bloom Filter** 中。若封包同時命中所有分類器的長尾過濾器，則交由穩定的 **KSet** 分類器處理，以降低長尾延遲。

### 3. RL-Based Multi-Classifier Parameter Optimization / 強化學習導向的分類器參數組合選擇

A three-stage reinforcement learning (RL) strategy optimizes classifier parameter configurations to maximize complementarity:

- **First Classifier**: The RL agent selects the best-performing classifier based on 99th percentile latency.
- **Second Classifier**: The agent minimizes the intersection of slow packets (exceeding P99 latency) between the first and second classifiers, ensuring high complementarity.
- **Third Classifier**: Extends the strategy to exclude slow packets from both prior classifiers, selecting a third classifier with minimal overlap.

Current implementation order: **PT-Tree → DBTable → DynamicTuple**. Future extensions can adjust ordering and combinations.

為提升分類器互補性，系統採用三階段強化學習（RL）搜尋策略最佳化參數配置：
- **第一分類器**：以第 99 百分位延遲為指標，選出最佳主分類器。
- **第二分類器**：針對第一分類器的慢封包（超過 P99 延遲），選擇與其慢封包交集最小的第二分類器，確保高互補性。
- **第三分類器**：排除前兩分類器的慢封包，選擇與其交集最小的第三分類器。

目前實作順序為 **PT-Tree → DBTable → DynamicTuple**，未來可拓展排序與組合。

---

## Directory Structure / 目錄結構

```plaintext
multiple_parallel_PC                      //
├─ .editorconfig                          //
├─ .mailmap                               //
├─ CMakeLists.txt                         //
├─ Dockerfile                             //
├─ INFO                                   //
│  ├─ BloomResults.txt                    //
│  ├─ DBT_IndivResults.txt                //
│  ├─ DT_IndivResults.txt                 //
│  ├─ KSet_IndivResults.txt               //
│  ├─ MT_IndivResults.txt                 //
│  ├─ PT_IndivResults.txt                 //
│  ├─ Total_knn_result.txt                //
│  ├─ Total_model_11_result.txt           //
│  ├─ Total_model_3_result.txt            //
│  ├─ Total_model_5_result.txt            //
│  ├─ Total_prediction_11_result.txt      //
│  ├─ Total_prediction_3_result.txt       //
│  ├─ Total_prediction_5_result.txt       //
│  ├─ Total_prediction_knn_result.txt     //
│  └─ param.txt                           //
├─ LICENSE                                //
├─ README.md                              //
├─ build                                  //
│  ├─ src                                 //
│  └─ tests                               //
│     └─ main                             //
├─ include                                //
│  ├─ DBT                                 //
│  │  ├─ DBT_core.hpp                     //
│  │  ├─ DBT_data_structure.hpp           //
│  │  └─ DBT_tss.hpp                      //
│  ├─ DT                                  //
│  │  ├─ dynamictuple-dims.h              //
│  │  ├─ dynamictuple-ranges.h            //
│  │  ├─ dynamictuple.h                   //
│  │  ├─ hash.h                           //
│  │  ├─ hashnode.h                       //
│  │  ├─ hashtable.h                      //
│  │  └─ tuple.h                          //
│  ├─ KNNClassifier.hpp                   //
│  ├─ KSet                                //
│  │  ├─ KSet.hpp                         //
│  │  ├─ KSet_data_structure.hpp          //
│  │  ├─ input.hpp                        //
│  │  └─ inputFile_test.hpp               //
│  ├─ LinearRegressionModel.hpp           //
│  ├─ MT                                  //
│  │  ├─ mhash.h                          //
│  │  ├─ mhashnode.h                      //
│  │  ├─ mhashtable.h                     //
│  │  ├─ mtuple.h                         //
│  │  └─ multilayertuple.h                //
│  ├─ PT                                  //
│  │  ├─ L3.txt                           //
│  │  ├─ PT_data_structure.hpp            //
│  │  └─ PT_tree.hpp                      //
│  ├─ basis.hpp                           //
│  ├─ bloomFilter.hpp                     //
│  ├─ bobHash.hpp                         //
│  ├─ elementary_DT_MT.h                  //
│  ├─ linearSearch_test.hpp               //
│  ├─ nanoflann.hpp                       //
│  └─ rl_gym.hpp                          //
├─ scripts                                //
│  ├─ build.sh                            //
│  ├─ cache_search_analyz.py              //
│  ├─ extract_twoColumns.py               //
│  ├─ part_0                              //
│  ├─ pcap_to_5tuple.cpp                  //
│  ├─ per_run.sh                          //
│  ├─ run.sh                              //
│  ├─ run_container.sh                    //
│  ├─ trace_append_rows.cpp               //
│  └─ trace_statics.py                    //
├─ src                                    //
│  ├─ CMakeLists.txt                      //
│  ├─ DBTable.cpp                         //
│  ├─ DTsrc                               //
│  │  ├─ dynamictuple-dims.cpp            //
│  │  ├─ dynamictuple-ranges.cpp          //
│  │  ├─ dynamictuple.cpp                 //
│  │  ├─ hash.cpp                         //
│  │  ├─ hashnode.cpp                     //
│  │  ├─ hashtable.cpp                    //
│  │  └─ tuple.cpp                        //
│  ├─ KSet.cpp                            //
│  ├─ MTsrc                               //
│  │  ├─ mhashnode.cpp                    //
│  │  ├─ mhashtable.cpp                   //
│  │  ├─ mtuple.cpp                       //
│  │  └─ multilayertuple.cpp              //
│  ├─ PT_tree.cpp                         //
│  └─ elementary_DT_MT.cpp                //
└─ tests                                  //
   ├─ CMakeLists.txt                      //
   ├─ main.cpp                            //
   ├─ rl_environment.py                   //
   ├─ rl_gym.cpp                          //
   └─ train.py                            //
../                               // 上一層資料夾
../classbench_set/ipv4-ruleset/acl1_100k     //
../classbench_set/ipv4-trace/acl1_100k_trace //
../classbench_set/ipv4-trace/part_0 //
```

---

## Build & Run / 編譯與執行

### Prerequisites / 先備套件

Install the required dependencies on a Linux system (e.g., Ubuntu):

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake libeigen3-dev libomp-dev python3-dev python3-pip git pybind11-dev
pip3 install --upgrade pip

pip3 install pybind11
# -I/home/user/.local/lib/python3.10/site-packages/pybind11/include -I/usr/include/python3.10
python3 -m pybind11 --includes

pip3 install gymnasium
# check
python3 -c "import gymnasium; print(gymnasium.__version__)"

pip3 install "ray[rllib]" torch
# check
python3 -c "import ray; print(ray.__version__)"

pip3 install numpy

# check
pip3 list | grep -E "pybind11|gymnasium|ray|torch|numpy"
# gymnasium                    1.0.0
# numpy                        2.1.3
# pybind11                     2.13.6
# ray                          2.46.0
# torch                        2.7.1

# Python 3.10.12
```

### One-Shot Script / 一鍵執行

Run the following command in the project root directory to build and execute benchmarks:

```bash
bash ./scripts/run.sh
# Or
bash ./scripts/per_run.sh
# acl5 cannot with part_0
# ipc2 cannot with part_0
```

- **English**: This script auto-detects CPU instruction sets, enables `-march=native -O3 -flto` and BMI2/AVX2 options, builds the project, runs default benchmarks, and outputs results.
- **中文**: 此腳本自動偵測 CPU 指令集，啟用 `-march=native -O3 -flto` 及 BMI2/AVX2 選項，完成建置後執行預設基準測試並輸出結果。

> **Note**: The `part_0` trace file is incompatible with `acl5` or `ipc2`. Use `./scripts/run.sh` to verify compatibility (`#define VALID # local trace_path="$TRACE_DIR/part_0"`).

> **注意**：`part_0` 追蹤檔案不支援 `acl5` 或 `ipc2`。請使用 `./tests/main.cpp` 和 `./scripts/run.sh` 確認檔案相容性（`{#define VALID} {# local trace_path="$TRACE_DIR/part_0"}`）。

---

## Performance Evaluation / 效能評估指標

### Metrics / 指標

| Metric            | Definition / 定義                          |
|-------------------|--------------------------------------------|
| **Construction Time** | Time to build classifier (ms) / 建構分類器所需時間 (ms) |
| **Lookup Time**   | Time to search (ns) / 查詢時間 (ns)        |
| **Memory Usage**  | Memory footprint (MB) / 記憶體占用 (MB)    |

- **English**: Benchmark results are exported as text reports for further analysis.
- **中文**: 測試結果以文字報表形式輸出，方便後續分析。

### Sample Result / 範例結果

Excerpt from `main_c_acl2.txt` (abridged):

```
[Omitted for brevity;]
```

---

## References / 參考文獻

- Y.-K. Chang et al., "Efficient Hierarchical Hash-Based Multi-Field Packet Classification With Fast Update for Software Switches," *IEEE Access*, vol. 13, pp. 28962-28978, 2025.
- Z. Liao et al., "PT-Tree: A Cascading Prefix Tuple Tree for Packet Classification in Dynamic Scenarios," *IEEE/ACM Transactions on Networking*, vol. 32, no. 1, pp. 506-519, 2024.
- Z. Liao et al., "DBTable: Leveraging Discriminative Bitsets for High-Performance Packet Classification," *IEEE/ACM Transactions on Networking*, vol. 32, no. 6, pp. 5232-5246, 2024.
- C. Zhang, G. Xie, X. Wang, "DynamicTuple: The Dynamic Adaptive Tuple for High-Performance Packet Classification," *Computer Networks*, vol. 202, 2022.
- C. Zhang, G. Xie, "MultilayerTuple: A General, Scalable and High-Performance Packet Classification Algorithm for SDN," *IFIP Networking*, 2021.

---

## License / 授權

This project is released under the [MIT License](LICENSE). Contributions via pull requests are welcome!

本專案採用 [MIT 授權](LICENSE)，歡迎透過 Pull Request 參與開發！

---

## Contact / 聯繫

For questions or contributions, please open an issue or submit a pull request on [GitHub](https://github.com/JiaChangGit/multiple_parallel_PC).

如有問題或欲貢獻程式碼，請在 [GitHub](https://github.com/JiaChangGit/multiple_parallel_PC) 上開啟 Issue 或提交 Pull Request。
