# # ------------------------------
# # Stage 1: 構建階段 (Builder Stage)
# # ------------------------------
# # 使用 ubuntu:22.04 作為基礎映像，提供 C++ 和 Python 開發環境
# FROM ubuntu:22.04 AS builder

# # 安裝基本套件，包含編譯工具、數學庫、Python 開發環境和網絡數據包處理庫
# # - build-essential, cmake, git, curl, wget: 用於編譯 C++ 代碼 (main.cpp, rl_gym.cpp, pcap_to_5tuple.cpp, trace_append_rows.cpp)
# # - libeigen3-dev, libomp-dev: 支持數學運算和並行計算
# # - python3, python3-pip, python3-dev, pybind11-dev: 支持 Python 腳本和 C++/Python 綁定
# # - libpcap-dev: 支持 pcap_to_5tuple.cpp 處理網絡數據包
# # - apt-get clean 和 rm -rf 清理緩存，減少映像大小
# RUN apt-get update && apt-get install -y \
#     build-essential \
#     cmake \
#     git \
#     curl \
#     wget \
#     libeigen3-dev \
#     libomp-dev \
#     python3 \
#     python3-pip \
#     python3-dev \
#     pybind11-dev \
#     libpcap-dev \
#     && apt-get clean \
#     && rm -rf /var/lib/apt/lists/* /var/cache/apt/*

# # 安裝 Python 套件，明確指定版本以確保可重現性
# # - pybind11: 用於 C++/Python 綁定 (rl_gym.cpp)
# # - gymnasium, ray[rllib]: 用於強化學習 (rl_environment.py, train.py)
# # - torch, numpy: 用於深度學習和數值計算
# # - scapy: 用於處理網絡數據包 (trace_statics.py, pcap_to_5tuple.cpp)
# # - --no-cache-dir: 避免緩存 pip 文件，減少映像大小
# # - --retries=10 --timeout=60: 提高安裝穩定性
# RUN pip3 install --no-cache-dir --retries=10 --timeout=60 \
#     pybind11==2.10.0 \
#     gymnasium==0.29.1 \
#     "ray[rllib]==2.9.0" \
#     torch==2.0.1 \
#     numpy==1.24.3 \
#     scapy

# # 設置工作目錄為 /workspace，便於管理項目文件
# WORKDIR /workspace

# # 複製 multiple_parallel_PC/ 目錄到 /workspace
# # 包含 CMakeLists.txt, scripts/, src/, tests/ 等
# # classbench_set/ 由 run_container.sh 通過卷掛載提供到 /workspace/classbench_set/
# COPY multiple_parallel_PC/ .

# # 編譯項目
# # - cmake -S . -B build: 使用 /workspace/CMakeLists.txt 進行配置，生成 build/ 目錄
# # - cmake --build build --target tests -j: 構建 tests 目標，生成 /workspace/build/tests/main
# # - -j: 並行編譯，加速構建
# RUN cmake -S . -B build && cmake --build build --target tests -j

# # 設置 run.sh 可執行權限，確保 CMD 可以運行
# RUN chmod +x /workspace/scripts/run.sh

# # ------------------------------
# # Stage 2: 運行階段 (Runtime Stage)
# # ------------------------------
# # 使用乾淨的 ubuntu:22.04 映像，僅包含運行時依賴
# FROM ubuntu:22.04

# # 安裝運行時依賴
# # - python3, python3-pip: 運行 Python 腳本
# # - libeigen3-dev, libomp-dev: 支持 C++ 可執行文件的數學和並行計算
# # - libpcap-dev: 支持 pcap_to_5tuple.cpp 和 trace_statics.py
# # - 清理緩存，減少映像大小
# RUN apt-get update && apt-get install -y \
#     python3 \
#     python3-pip \
#     libeigen3-dev \
#     libomp-dev \
#     libpcap-dev \
#     && apt-get clean \
#     && rm -rf /var/lib/apt/lists/* /var/cache/apt/*

# # 安裝運行時 Python 套件，與構建階段一致
# RUN pip3 install --no-cache-dir --retries=10 --timeout=60 \
#     pybind11==2.10.0 \
#     gymnasium==0.29.1 \
#     "ray[rllib]==2.9.0" \
#     torch==2.0.1 \
#     numpy==1.24.3 \
#     scapy

# # 設置工作目錄為 /workspace
# WORKDIR /workspace

# # 從構建階段複製必要文件
# # - /workspace/build: 包含編譯後的可執行文件 (如 build/tests/main)
# # - /workspace/scripts: 包含 run.sh, build.sh, pcap_to_5tuple.cpp 等
# # - /workspace/src: 包含源代碼 (可能包含 CMakeLists.txt)
# # - /workspace/tests: 包含 main.cpp, rl_environment.py, train.py 等
# # - /workspace/CMakeLists.txt: 保留根目錄的 CMake 配置
# COPY --from=builder /workspace/build /workspace/build
# COPY --from=builder /workspace/scripts /workspace/scripts
# COPY --from=builder /workspace/src /workspace/src
# COPY --from=builder /workspace/tests /workspace/tests
# COPY --from=builder /workspace/CMakeLists.txt /workspace/

# # 設置環境變量
# # - PYTHONPATH: 包含 /workspace, /workspace/tests, /workspace/scripts，確保 Python 腳本能找到模組
# # - LD_LIBRARY_PATH: 包含 /usr/local/lib 和 /usr/lib，確保 C++ 可執行文件能找到動態庫
# ENV PYTHONPATH=/workspace:/workspace/tests:/workspace/scripts:$PYTHONPATH
# ENV LD_LIBRARY_PATH=/usr/local/lib:/usr/lib:$LD_LIBRARY_PATH

# # 設置 run.sh 可執行權限（再次確保）
# RUN chmod +x /workspace/scripts/run.sh

# # 預設執行 run.sh，作為容器啟動的入口
# # - C++ 程式碼中的 ../classbench_set/ 將解析到 /workspace/classbench_set/
# CMD ["bash", "/workspace/scripts/run.sh"]


# ------------------------------
# Stage 1: 構建階段 (Builder Stage)
# ------------------------------
# 使用 ubuntu:22.04 作為基礎映像，提供 C++ 和 Python 開發環境
FROM ubuntu:22.04 AS builder

# 安裝基本套件，包含編譯工具、數學庫、Python 開發環境和網絡數據包處理庫
# - build-essential, cmake, git, curl, wget: 用於編譯 C++ 代碼 (main.cpp, rl_gym.cpp, pcap_to_5tuple.cpp, trace_append_rows.cpp)
# - libeigen3-dev, libomp-dev: 支持數學運算和並行計算
# - python3, python3-pip, python3-dev, pybind11-dev: 支持 Python 腳本和 C++/Python 綁定
# - libpcap-dev: 支持 pcap_to_5tuple.cpp 處理網絡數據包
# - apt-get clean 和 rm -rf 清理緩存，減少映像大小
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    curl \
    wget \
    libeigen3-dev \
    libomp-dev \
    python3 \
    python3-pip \
    python3-dev \
    pybind11-dev \
    libpcap-dev \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/* /var/cache/apt/*

# 安裝 Python 套件，明確指定版本以確保可重現性
# - pybind11: 用於 C++/Python 綁定 (rl_gym.cpp)
# - gymnasium, ray[rllib]: 用於強化學習 (rl_environment.py, train.py)
# - torch, numpy: 用於深度學習和數值計算
# - scapy: 用於處理網絡數據包 (trace_statics.py, pcap_to_5tuple.cpp)
# - --no-cache-dir: 避免緩存 pip 文件，減少映像大小
# - --retries=10 --timeout=60: 提高安裝穩定性
RUN pip3 install --no-cache-dir --retries=10 --timeout=60 \
    pybind11==2.10.0 \
    gymnasium==0.29.1 \
    "ray[rllib]==2.9.0" \
    torch==2.0.1 \
    numpy==1.24.3 \
    scapy

# 設置工作目錄為 /workspace/pc，便於管理項目文件
WORKDIR /workspace/pc

# 複製 multiple_parallel_PC/ 目錄到 /workspace/pc
# 包含 CMakeLists.txt, scripts/, src/, tests/ 等
# classbench_set/ 由 run_container.sh 通過卷掛載提供到 /workspace/classbench_set/
COPY multiple_parallel_PC/ .

# 編譯項目
# - cmake -S . -B build: 使用 /workspace/pc/CMakeLists.txt 進行配置，生成 build/ 目錄
# - cmake --build build --target tests -j: 構建 tests 目標，生成 /workspace/pc/build/tests/main
# - -j: 並行編譯，加速構建
RUN cmake -S . -B build && cmake --build build --target tests -j

# 設置 run.sh 可執行權限，確保 CMD 可以運行
RUN chmod +x /workspace/pc/scripts/run.sh

# ------------------------------
# Stage 2: 運行階段 (Runtime Stage)
# ------------------------------
# 使用乾淨的 ubuntu:22.04 映像，僅包含運行時依賴
FROM ubuntu:22.04

# 安裝運行時依賴
# - python3, python3-pip: 運行 Python 腳本
# - libeigen3-dev, libomp-dev: 支持 C++ 可執行文件的數學和並行計算
# - libpcap-dev: 支持 pcap_to_5tuple.cpp 和 trace_statics.py
# - 清理緩存，減少映像大小
RUN apt-get update && apt-get install -y \
    python3 \
    python3-pip \
    libeigen3-dev \
    libomp-dev \
    libpcap-dev \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/* /var/cache/apt/*

# 安裝運行時 Python 套件，與構建階段一致
RUN pip3 install --no-cache-dir --retries=10 --timeout=60 \
    pybind11==2.10.0 \
    gymnasium==0.29.1 \
    "ray[rllib]==2.9.0" \
    torch==2.0.1 \
    numpy==1.24.3 \
    scapy

# 設置工作目錄為 /workspace/pc
WORKDIR /workspace/pc

# 從構建階段複製必要文件
# - /workspace/pc/build: 包含編譯後的可執行文件 (如 build/tests/main)
# - /workspace/pc/scripts: 包含 run.sh, build.sh, pcap_to_5tuple.cpp 等
# - /workspace/pc/src: 包含源代碼 (可能包含 CMakeLists.txt)
# - /workspace/pc/tests: 包含 main.cpp, rl_environment.py, train.py 等
# - /workspace/pc/CMakeLists.txt: 保留根目錄的 CMake 配置
COPY --from=builder /workspace/pc/build /workspace/pc/build
COPY --from=builder /workspace/pc/scripts /workspace/pc/scripts
COPY --from=builder /workspace/pc/src /workspace/pc/src
COPY --from=builder /workspace/pc/tests /workspace/pc/tests
COPY --from=builder /workspace/pc/CMakeLists.txt /workspace/pc/

# 設置環境變量
# - PYTHONPATH: 包含 /workspace/pc, /workspace/pc/tests, /workspace/pc/scripts，確保 Python 腳本能找到模組
# - LD_LIBRARY_PATH: 包含 /usr/local/lib 和 /usr/lib，確保 C++ 可執行文件能找到動態庫
ENV PYTHONPATH=/workspace/pc:/workspace/pc/tests:/workspace/pc/scripts:$PYTHONPATH
ENV LD_LIBRARY_PATH=/usr/local/lib:/usr/lib:$LD_LIBRARY_PATH

# 設置 run.sh 可執行權限（再次確保）
RUN chmod +x /workspace/pc/scripts/run.sh

# 預設執行 run.sh，作為容器啟動的入口
# - C++ 程式碼中的 ../classbench_set/ 將解析到 /workspace/classbench_set/
CMD ["bash", "/workspace/pc/scripts/run.sh"]
