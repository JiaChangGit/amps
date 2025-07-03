# ------------------------------
# Stage 1: 構建階段 (Builder Stage)
# ------------------------------
# 使用 Ubuntu 22.04 作為基礎映像，用於構建項目
FROM ubuntu:22.04 AS builder

# 安裝構建所需的依賴包
# - build-essential: 包含 gcc、g++ 等編譯工具
# - cmake: 用於生成構建文件
# - libeigen3-dev, libomp-dev, libpcap-dev: C++ 依賴庫
# - python3 相關套件: 用於 Python 環境和 pybind11
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

# 安裝 Python 套件
# - pybind11: 用於 C++ 和 Python 之間的綁定
# - gymnasium, ray[rllib], torch: 機器學習相關庫
# - numpy, scapy: 數值計算和網絡封包分析
RUN pip3 install --no-cache-dir --retries=10 --timeout=60 \
    pybind11==2.13.6 \
    gymnasium==1.0.0 \
    "ray[rllib]==2.46.0" \
    torch==2.7.1 \
    numpy==2.1.3 \
    scapy

# 設置工作目錄
# 所有後續操作都在 /workspace/pc 下進行
WORKDIR /workspace/pc

# 複製構建上下文中的所有文件到容器
# - 假設 CMakeLists.txt、scripts/、src/、tests/ 位於構建上下文根目錄（例如 C:\Users\user\Desktop\ccpc）
COPY . .

# 構建項目
# - cmake -S . -B build: 生成構建文件到 build/ 目錄
# - cmake --build: 編譯項目，目標為 tests，使用多核 (-j)
RUN cmake -S . -B build && cmake --build build --target tests -j

# 設置 run.sh 的可執行權限
RUN chmod +x /workspace/pc/scripts/run.sh

# ------------------------------
# Stage 2: 運行階段 (Runtime Stage)
# ------------------------------
# 使用新的 Ubuntu 22.04 映像作為運行時環境
FROM ubuntu:22.04

# 安裝運行時依賴
# - 只保留必要的運行時庫，減少映像大小
RUN apt-get update && apt-get install -y \
    python3 \
    python3-pip \
    libeigen3-dev \
    libomp-dev \
    libpcap-dev \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/* /var/cache/apt/*

# 安裝運行時 Python 套件
RUN pip3 install --no-cache-dir --retries=10 --timeout=60 \
    pybind11 \
    gymnasium \
    "ray[rllib]" \
    torch \
    numpy \
    scapy

# 設置工作目錄
WORKDIR /workspace/pc

# 從構建階段複製必要文件到運行階段
COPY --from=builder /workspace/pc/build /workspace/pc/build
COPY --from=builder /workspace/pc/scripts /workspace/pc/scripts
COPY --from=builder /workspace/pc/src /workspace/pc/src
COPY --from=builder /workspace/pc/tests /workspace/pc/tests
COPY --from=builder /workspace/pc/CMakeLists.txt /workspace/pc/

# 設置環境變量
# - PYTHONPATH: 指定 Python 模組搜索路徑
# - LD_LIBRARY_PATH: 指定動態庫搜索路徑
# - 使用 ${VAR:+:$VAR} 語法避免未定義變量的警告
ENV PYTHONPATH=/workspace/pc:/workspace/pc/tests:/workspace/pc/scripts:/usr/local/lib/python3.10/dist-packages${PYTHONPATH:+:$PYTHONPATH}
ENV LD_LIBRARY_PATH=/usr/local/lib:/usr/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}

# 設置 run.sh 的可執行權限
RUN chmod +x /workspace/pc/scripts/run.sh

# 默認運行命令
# - 啟動容器時執行 run.sh
CMD ["bash", "/workspace/pc/scripts/run.sh"]
