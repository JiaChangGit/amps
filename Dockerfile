## 如何使用
# 在 multiple_parallel_PC 目錄下構建映像：
#   docker build -t multiple_parallel_pc .
# 運行容器：
#   Windows (Git Bash): ./scripts/run_container.sh
#   Ubuntu: chmod +x ./scripts/run_container.sh && ./scripts/run_container.sh

# ------------------------------
# Stage 1: 基礎環境與建置
# ------------------------------
FROM ubuntu:22.04

# 基本套件
RUN apt-get update && apt-get install -y \
    build-essential cmake git curl wget \
    libeigen3-dev libomp-dev \
    python3 python3-pip python3-dev \
    pybind11-dev \
    && rm -rf /var/lib/apt/lists/*

# Python 套件（使用 retry 與快取）
ENV PIP_CACHE_DIR=/root/.cache/pip
RUN pip3 install --upgrade pip \
 && pip3 install --retries=10 --timeout=60 --progress-bar off \
    pybind11 \
    gymnasium \
    "ray[rllib]" \
    torch \
    numpy

# 建立工作目錄
WORKDIR /workspace/scripts

# 將整個專案複製進去（只包含 multiple_parallel_PC）
COPY . /workspace

# 編譯（產生 build/tests/main）
RUN cmake -S .. -B ../build && cmake --build ../build --target tests -j

# 預設進 container 就執行 run.sh
CMD ["bash", "run.sh"]
