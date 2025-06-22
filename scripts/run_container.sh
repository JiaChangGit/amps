#!/bin/bash

## 如何使用
# 在 multiple_parallel_PC 目錄下
# 先構建 Docker 映像：
#   docker build -t multiple_parallel_pc .
# 然後運行容器：
#   Windows (Git Bash): ./scripts/run_container.sh
#   Ubuntu: chmod +x ./scripts/run_container.sh && ./scripts/run_container.sh
# 手動運行容器：
#   Windows (Git Bash): export MSYS_NO_PATHCONV=1 && docker run --rm -it -v "$(pwd)/../classbench_set:/workspace/classbench_set" -w /workspace/scripts multiple_parallel_pc
#   Ubuntu: docker run --rm -it -v "$(pwd)/../classbench_set:/workspace/classbench_set" -w /workspace/scripts multiple_parallel_pc

# ===============================
# Script to run Docker container
# ===============================

# 設定 image 名稱
IMAGE_NAME="multiple_parallel_pc"

# 禁用 Git Bash 路徑轉換（僅 Windows 需要）
if [[ "$OSTYPE" == "msys" || "$OSTYPE" == "win32" || "$OSTYPE" == "cygwin" ]]; then
    export MSYS_NO_PATHCONV=1
fi

# 獲取當前目錄（跨平台相容）
if [ -n "$PWD" ]; then
    CURRENT_DIR="$PWD"
else
    CURRENT_DIR="$(pwd)"
fi

# 預設 classbench_set 目錄（相對路徑）
CLASSBENCH_DIR="${CURRENT_DIR}/../classbench_set"

# 將相對路徑轉為絕對路徑
if ! CLASSBENCH_DIR=$(realpath "$CLASSBENCH_DIR" 2>/dev/null || readlink -f "$CLASSBENCH_DIR" 2>/dev/null); then
    echo "[錯誤] 無法解析 classbench_set 路徑：$CLASSBENCH_DIR"
    exit 1
fi

# 檢查 classbench_set 是否存在
if [ ! -d "$CLASSBENCH_DIR" ]; then
    echo "[錯誤] 未找到 classbench_set 目錄：$CLASSBENCH_DIR"
    echo "請確保該目錄存在於專案根目錄的上層。"
    exit 1
fi

# 檢查 image 是否存在
if ! docker image inspect "$IMAGE_NAME" >/dev/null 2>&1; then
    echo "[錯誤] 未找到 Docker 映像 '$IMAGE_NAME'。"
    echo "請先構建映像，使用以下命令："
    echo "  docker build -t $IMAGE_NAME ."
    exit 2
fi

# 啟動 container：掛載到 /workspace/classbench_set，並設置工作目錄為 /workspace/scripts
echo "[資訊] 正在啟動容器，映像名稱：$IMAGE_NAME"
docker run --rm -it \
    -v "$CLASSBENCH_DIR:/workspace/classbench_set" \
    -w /workspace/scripts \
    "$IMAGE_NAME"
