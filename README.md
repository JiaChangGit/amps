multiple_parallel_PC
A High-Performance, Multi-Classifier Packet Classification Framework with Latency Prediction and Complementary Optimization高效能多分類器封包分類架構，結合延遲預測與互補性最佳化

Overview / 專案概觀
Englishmultiple_parallel_PC is a high-performance packet classification architecture implemented in C++17 and Eigen3. It is designed for multiclassifier collaboration and supports five heterogeneous and complementary packet classification data structures: KSet, PT-Tree, DBTable, DynamicTuple, and MultilayerTuple. The core design explores multiple classifier choices through two independent mechanisms.
中文multiple_parallel_PC 是一套以 C++17 與 Eigen3 實作的高效能封包分類架構，專為多分類器協同運作設計，支持五種具異質性與互補性的封包分類資料結構：KSet、PT-Tree、DBTable、DynamicTuple 及 MultilayerTuple。其核心設計透過兩個獨立機制探討多分類器選擇。

Key Mechanisms / 核心機制
1. Latency-Aware Classifier Prediction / 延遲預測導向的分類器選擇
EnglishThe framework employs a multidimensional linear regression model that uses a packet's five-tuple (source IP, destination IP, source port, destination port, transport protocol) to predict the query latency for each classifier (PT-Tree, DBTable, DynamicTuple). It dynamically selects the classifier with the lowest predicted latency for each packet, improving average query performance and avoiding degradation under high-variability traffic.
中文本系統利用多維線性回歸模型，以封包的五維欄位（來源 IP、目的 IP、來源 port、目的 port、傳輸層協定）為輸入，預測該封包在各分類器（PT-Tree、DBTable、DynamicTuple）的查詢延遲，並動態選擇延遲預期最小者進行分類。此動態選擇機制可即時因應封包特徵，提升平均查詢效能，避免單一分類器在高變異度流量下的效能劣化。
2. Long-Tail-Aware Filtering / 長尾封包感知式過濾機制
EnglishTo handle high-latency tail packets, each classifier (PT-Tree, DBTable, DynamicTuple, MultilayerTuple) identifies packets exceeding the 99th percentile query latency threshold as slow packets. These packets' five-tuple signatures are stored in a Bloom Filter, Cuckoo Filter, or Elastic Bloom Filter. If a packet matches the long-tail filters of all classifiers, indicating high query latency across them, it is routed to the stable KSet classifier to mitigate tail latency.
中文為處理高查詢延遲的長尾封包（tail packets），每個分類器（PT-Tree、DBTable、DynamicTuple、MultilayerTuple）根據查詢行為特徵，將超過第 99 百分位查詢延遲門檻的封包識別為 slow packets，並將其五維欄位構成 signature，插入對應的 Bloom Filter、Cuckoo Filter 或 Elastic Bloom Filter。若某封包同時命中所有分類器的長尾過濾器，則表示其查詢延遲均偏高，此時會交由效能穩定的 KSet 處理，以降低長尾延遲。
3. RL-Based Multi-Classifier Parameter Optimization / 強化學習導向的分類器參數組合選擇
EnglishTo enhance classifier complementarity, a three-stage reinforcement learning (RL) search strategy optimizes parameter configurations:

First Classifier: The RL agent evaluates the 99th percentile latency to select the best-performing classifier.
Second Classifier: Slow packets from the first classifier (exceeding P99 latency) are identified. The RL agent selects the second classifier by minimizing the intersection of slow packets with the first classifier, ensuring high complementarity.
Third Classifier: Extends the strategy to exclude slow packets from both prior classifiers, selecting the third classifier with minimal intersection for optimal complementarity.Current implementation order: PT-Tree → DBTable → DynamicTuple. Future extensions can adjust ordering and combinations.

中文為提升分類器參數組合的互補性，本架構導入三階段強化學習（RL）搜尋策略：

第一分類器：以查詢延遲第 99 百分位為指標，RL agent 探索所有參數設定，挑選延遲表現最優者作為主分類器。
第二分類器：將第一分類器查詢延遲高於 P99 門檻的 slow packets 視為需互補區間，根據第二分類器的 slow packets 記錄進行交集排除，產生 diff_first_vector。向量越小，互補性越高，作為 RL agent 的 reward signal，選出最佳第二參數。
第三分類器：排除第一與第二分類器皆不擅長的 slow packets，與第三分類器的 slow packets 進行交集運算，得出 diff_second_vector。向量越小，互補效果越好，適合作為第三分類器。目前實作順序為 PT-Tree → DBTable → DynamicTuple，未來可依需求拓展排序與組合。


Features / 特色



Feature / 特色
Description / 說明



Adaptive Dispatch
Linear regression or Bloom filter predicts the most efficient classifier in ~10 ns.


Multi-Classifier Core
Supports KSet, PT-Tree, DBTable, DynamicTuple, and MultilayerTuple.


SIMD / PEXT Optimization
Uses AVX2/SSE2 intrinsics, BMI2 PEXT, and cache-aligned data structures for maximum throughput.


Transparent Metrics
Automatically measures construction time, throughput (pps), and memory footprint.


Scripted Benchmarking
scripts/run.sh automates building, running, and reporting on synthetic or trace-driven workloads.



Directory Structure / 目錄結構
multiple_parallel_PC                      //
├─ .editorconfig                          //
├─ .mailmap                               //
├─ CMakeLists.txt                         //
├─ INFO                                   //
│  ├─ BloomResults.txt                    //
│  ├─ DBT_IndivResults.txt                //
│  ├─ DBT_buckets.txt                     //
│  ├─ DBT_nodes.txt                       //
│  ├─ DBT_prediction_3_result.txt         //
│  ├─ DT_IndivResults.txt                 //
│  ├─ DT_prediction_3_result.txt          //
│  ├─ KSet_IndivResults.txt               //
│  ├─ KSet_prediction_3_result.txt        //
│  ├─ MT_IndivResults.txt                 //
│  ├─ MT_prediction_3_result.txt          //
│  ├─ PT_IndivResults.txt                 //
│  ├─ PT_prediction_3_result.txt          //
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
│  ├─ main_c_acl1.txt                     //
│  ├─ main_c_acl2.txt                     //
│  ├─ src                                 //
│  └─ tests                               //
│     └─ main                             //
├─ file_structure                         //
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
   ├─ checkpoints                         //
   │  ├─ algorithm_state.pkl              //
   │  ├─ policies                         //
   │  │  └─ default_policy                //
   │  │     ├─ policy_state.pkl           //
   │  │     └─ rllib_checkpoint.json      //
   │  └─ rllib_checkpoint.json            //
   ├─ main.cpp                            //
   ├─ ray_results                         //
   │  └─ ppo_tensorboard                  //
   │     ├─ params.json                   //
   │     ├─ params.pkl                    //
   │     ├─ progress.csv                  //
   │     └─ result.json                   //
   ├─ rl_environment.py                   //
   ├─ rl_gym.cpp                          //
   └─ train.py                            //


Build & Run / 編譯與執行
Prerequisites / 先備套件
sudo apt-get update
sudo apt-get install -y build-essential cmake libeigen3-dev libomp-dev

One-Shot Script / 一鍵執行
# In the project root directory
sudo bash ./scripts/run.sh

EnglishThis script auto-detects CPU instruction sets, enables -march=native -O3 -flto and BMI2/AVX2 options, builds the project, runs default benchmarks, and outputs results.
中文此腳本自動偵測 CPU 指令集，啟用 -march=native -O3 -flto 及 BMI2/AVX2 選項，完成建置後執行預設基準測試並輸出結果。
Noticepart_0 cannot use acl5 or ipc2. Use ./scripts/run.sh to verify if the trace file is compatible (#define VALID # local trace_path="$TRACE_DIR/part_0").您可以使用 ./scripts/run.sh 確認 trace file 是否可用 (#define VALID # local trace_path="$TRACE_DIR/part_0")。

Performance Evaluation / 效能評估指標



Metric / 指標
Definition / 定義



Construction Time
Time to build classifier (ms) / 建構分類器所需時間 (ms)


Lookup Throughput
Query rate (Mpps) / 查詢吞吐量 (Mpps)


Memory Usage
Memory footprint (MB) / 記憶體占用 (MB)


EnglishBenchmark results are exported as text reports for further analysis.
中文測試結果以 txt 格式輸出，方便後續分析。

Result / 結果
main_c_acl2.txt (abridged):
[Omitted for brevity; refer to the file for full details]


References / 參考文獻

Y.-K. Chang et al. "Efficient Hierarchical Hash-Based Multi-Field Packet Classification With Fast Update for Software Switches," IEEE Access, vol. 13, pp. 28962‑28978, 2025.
Z. Liao et al. "PT‑Tree: A Cascading Prefix Tuple Tree for Packet Classification in Dynamic Scenarios," IEEE/ACM Transactions on Networking, vol. 32, no. 1, pp. 506‑519, 2024.
Z. Liao et al. "DBTable: Leveraging Discriminative Bitsets for High‑Performance Packet Classification," IEEE/ACM Transactions on Networking, vol. 32, no. 6, pp. 5232‑5246, 2024.
C. Zhang, G. Xie, X. Wang. "DynamicTuple: The Dynamic Adaptive Tuple for High‑Performance Packet Classification," Computer Networks, vol. 202, 2022.
C. Zhang, G. Xie. "MultilayerTuple: A General, Scalable and High‑Performance Packet Classification Algorithm for SDN," IFIP Networking, 2021.


License / 授權
EnglishReleased under the MIT License. Contributions via pull requests are welcome!
中文本專案採用 MIT 授權，歡迎提交 Pull Request 共同參與開發！


EnglishFor more details, refer to the source code and scripts in this repository.**中文詳細內容請參考本專案原始碼與腳本。

