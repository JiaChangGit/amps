#!/bin/bash

# ===============================
# 腳本：運行 Docker 容器
# ===============================
# 目的：啟動 Docker 容器，將主機的 classbench_set/ 目錄掛載到容器內的 /workspace/classbench_set/
# 前提：必須在 ccpc/ 目錄下運行此腳本，例如 C:\Users\user\Desktop\ccpc
#       classbench_set/ 應位於 ccpc/ 的上層目錄（例如 C:\Users\user\Desktop\classbench_set）

# 設置 Docker 映像名稱
IMAGE_NAME="multiple_parallel_pc"

# 禁用 Git Bash 路徑轉換（僅適用於 Windows 的 Git Bash）
if [[ "$OSTYPE" == "msys" || "$OSTYPE" == "win32" || "$OSTYPE" == "cygwin" ]]; then
    export MSYS_NO_PATHCONV=1
fi

# 獲取當前目錄（跨平台兼容）
CURRENT_DIR="${PWD:-$(pwd)}"

# 設置 classbench_set 目錄（相對於 ccpc/ 的上層）
CLASSBENCH_DIR="${CURRENT_DIR}/../classbench_set"

# 轉換為絕對路徑，確保兼容性
if ! CLASSBENCH_DIR=$(python3 -c "import os; print(os.path.abspath('$CLASSBENCH_DIR'))" 2>/dev/null); then
    echo "[錯誤] 無法解析 classbench_set 路徑：$CLASSBENCH_DIR"
    echo "請確保 Python3 已安裝並可執行，或檢查路徑是否正確。"
    exit 1
fi

# 檢查 classbench_set 目錄是否存在
if [ ! -d "$CLASSBENCH_DIR" ]; then
    echo "[錯誤] 未找到 classbench_set 目錄：$CLASSBENCH_DIR"
    echo "請確保該目錄存在於 ccpc/ 的上層目錄。"
    echo "當前目錄：$CURRENT_DIR"
    exit 1
fi

# 檢查 Docker 映像是否存在
if ! docker image inspect "$IMAGE_NAME" >/dev/null 2>&1; then
    echo "[錯誤] 未找到 Docker 映像 '$IMAGE_NAME'。"
    echo "請在 ccpc/ 目錄下先構建映像，使用以下命令："
    echo "  docker build -t $IMAGE_NAME ."
    exit 2
fi

# 啟動容器
# - --rm: 容器退出後自動刪除
# - -it: 啟用交互模式和終端
# - -v: 掛載 classbench_set/ 到 /workspace/classbench_set/
# - -w: 設置工作目錄為 /workspace/pc/scripts
echo "[資訊] 正在啟動容器，映像名稱：$IMAGE_NAME"
echo "[資訊] 掛載 classbench_set 目錄：$CLASSBENCH_DIR -> /workspace/classbench_set"
if ! docker run --rm -it \
    -v "$CLASSBENCH_DIR:/workspace/classbench_set" \
    -w /workspace/pc/scripts \
    "$IMAGE_NAME"; then
    echo "[錯誤] 容器啟動失敗，請檢查 run.sh 或容器內部配置。"
    exit 3
fi
