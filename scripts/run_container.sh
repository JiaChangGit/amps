# #!/bin/bash

# # ===============================
# # Script to run Docker container
# # ===============================
# # 功能：啟動 Docker 容器，將主機的 classbench_set/ 掛載到容器內的 /workspace/classbench_set/
# #       以匹配 C++ 程式碼中的相對路徑 ../classbench_set/
# # 前提：必須在包含 classbench_set/ 和 multiple_parallel_PC/ 的目錄執行
# #       例如：cd .. && multiple_parallel_PC/scripts/run_container.sh
# # 相依：Docker 映像 multiple_parallel_pc 已構建
# #       classbench_set/ 位於 multiple_parallel_PC/ 的上層目錄

# # 設定映像名稱
# IMAGE_NAME="multiple_parallel_pc"

# # 禁用 Git Bash 路徑轉換（僅 Windows 的 Git Bash 需要）
# # 適用於 msys, win32, cygwin 環境
# if [[ "$OSTYPE" == "msys" || "$OSTYPE" == "win32" || "$OSTYPE" == "cygwin" ]]; then
#     export MSYS_NO_PATHCONV=1
# fi

# # 獲取當前目錄（跨平台兼容）
# if [ -n "$PWD" ]; then
#     CURRENT_DIR="$PWD"
# else
#     CURRENT_DIR="$(pwd)"
# fi

# # 設定 classbench_set 目錄（相對路徑：multiple_parallel_PC/ 的上層）
# CLASSBENCH_DIR="${CURRENT_DIR}/../classbench_set"

# # 將相對路徑轉為絕對路徑，兼容 Linux, macOS 和 Git Bash
# # 使用 python3 替代 realpath/readlink，確保跨平台支持
# if ! CLASSBENCH_DIR=$(python3 -c "import os; print(os.path.abspath('$CLASSBENCH_DIR'))" 2>/dev/null); then
#     echo "[錯誤] 無法解析 classbench_set 路徑：$CLASSBENCH_DIR"
#     echo "請確認 Python3 已安裝並可執行，或檢查路徑是否正確。"
#     exit 1
# fi

# # 檢查 classbench_set 目錄是否存在
# if [ ! -d "$CLASSBENCH_DIR" ]; then
#     echo "[錯誤] 未找到 classbench_set 目錄：$CLASSBENCH_DIR"
#     echo "請確保該目錄存在於 multiple_parallel_PC/ 的上層目錄。"
#     echo "當前目錄：$CURRENT_DIR"
#     exit 1
# fi

# # 檢查 Docker 映像是否存在
# if ! docker image inspect "$IMAGE_NAME" >/dev/null 2>&1; then
#     echo "[錯誤] 未找到 Docker 映像 '$IMAGE_NAME'。"
#     echo "請在 multiple_parallel_PC/ 目錄下先構建映像，使用以下命令："
#     echo "  docker build -t $IMAGE_NAME ."
#     exit 2
# fi

# # 啟動容器
# # - --rm: 容器退出後自動刪除
# # - -it: 啟用交互模式和終端
# # - -v: 將主機的 classbench_set/ 掛載到容器內的 /workspace/classbench_set/
# # - -w: 設置工作目錄為 /workspace/scripts，與 Dockerfile 和 run.sh 一致
# # - $IMAGE_NAME: 使用指定的映像名稱
# echo "[資訊] 正在啟動容器，映像名稱：$IMAGE_NAME"
# echo "[資訊] 掛載 classbench_set 目錄：$CLASSBENCH_DIR -> /workspace/classbench_set"
# if ! docker run --rm -it \
#     -v "$CLASSBENCH_DIR:/workspace/classbench_set" \
#     -w /workspace/scripts \
#     "$IMAGE_NAME"; then
#     echo "[錯誤] 容器啟動失敗，請檢查 run.sh 或容器內部配置。"
#     exit 3
# fi


#!/bin/bash

# ===============================
# Script to run Docker container
# ===============================
# 功能：啟動 Docker 容器，將主機的 classbench_set/ 掛載到容器內的 /workspace/classbench_set/
#       以匹配 C++ 程式碼中的相對路徑 ../classbench_set/
# 前提：必須在包含 classbench_set/ 和 multiple_parallel_PC/ 的目錄執行
#       例如：cd .. && multiple_parallel_PC/scripts/run_container.sh
# 相依：Docker 映像 multiple_parallel_pc 已構建
#       classbench_set/ 位於 multiple_parallel_PC/ 的上層目錄

# 設定映像名稱
IMAGE_NAME="multiple_parallel_pc"

# 禁用 Git Bash 路徑轉換（僅 Windows 的 Git Bash 需要）
# 適用於 msys, win32, cygwin 環境
if [[ "$OSTYPE" == "msys" || "$OSTYPE" == "win32" || "$OSTYPE" == "cygwin" ]]; then
    export MSYS_NO_PATHCONV=1
fi

# 獲取當前目錄（跨平台兼容）
if [ -n "$PWD" ]; then
    CURRENT_DIR="$PWD"
else
    CURRENT_DIR="$(pwd)"
fi

# 設定 classbench_set 目錄（相對路徑：multiple_parallel_PC/ 的上層）
CLASSBENCH_DIR="${CURRENT_DIR}/../classbench_set"

# 將相對路徑轉為絕對路徑，兼容 Linux, macOS 和 Git Bash
# 使用 python3 替代 realpath/readlink，確保跨平台支持
if ! CLASSBENCH_DIR=$(python3 -c "import os; print(os.path.abspath('$CLASSBENCH_DIR'))" 2>/dev/null); then
    echo "[錯誤] 無法解析 classbench_set 路徑：$CLASSBENCH_DIR"
    echo "請確認 Python3 已安裝並可執行，或檢查路徑是否正確。"
    exit 1
fi

# 檢查 classbench_set 目錄是否存在
if [ ! -d "$CLASSBENCH_DIR" ]; then
    echo "[錯誤] 未找到 classbench_set 目錄：$CLASSBENCH_DIR"
    echo "請確保該目錄存在於 multiple_parallel_PC/ 的上層目錄。"
    echo "當前目錄：$CURRENT_DIR"
    exit 1
fi

# 檢查 Docker 映像是否存在
if ! docker image inspect "$IMAGE_NAME" >/dev/null 2>&1; then
    echo "[錯誤] 未找到 Docker 映像 '$IMAGE_NAME'。"
    echo "請在 multiple_parallel_PC/ 目錄下先構建映像，使用以下命令："
    echo "  docker build -t $IMAGE_NAME ."
    exit 2
fi

# 啟動容器
# - --rm: 容器退出後自動刪除
# - -it: 啟用交互模式和終端
# - -v: 將主機的 classbench_set/ 掛載到容器內的 /workspace/classbench_set/
# - -w: 設置工作目錄為 /workspace/pc/scripts，與 Dockerfile 和 run.sh 一致
# - $IMAGE_NAME: 使用指定的映像名稱
echo "[資訊] 正在啟動容器，映像名稱：$IMAGE_NAME"
echo "[資訊] 掛載 classbench_set 目錄：$CLASSBENCH_DIR -> /workspace/classbench_set"
if ! docker run --rm -it \
    -v "$CLASSBENCH_DIR:/workspace/classbench_set" \
    -w /workspace/pc/scripts \
    "$IMAGE_NAME"; then
    echo "[錯誤] 容器啟動失敗，請檢查 run.sh 或容器內部配置。"
    exit 3
fi
