#!/bin/bash

## HOW to use
# chmod +x ./scripts/run_container.sh
# ./scripts/run_container.sh
# Or
# bash ./scripts/run_container.sh

# === Auto-run Docker container with volume mounting ===

# 判斷 classbench_set 是否存在於上一層
if [ ! -d ../classbench_set ]; then
    echo "[ERROR] ../classbench_set not found!"
    echo "Please make sure classbench_set is in the correct directory."
    exit 1
fi

# Container image name
IMAGE_NAME="multiple_parallel_pc"

# 檢查 image 是否存在，若不存在提示使用者先 build
if ! docker image inspect "$IMAGE_NAME" >/dev/null 2>&1; then
    echo "[ERROR] Docker image '$IMAGE_NAME' not found."
    echo "Please build it first using:"
    echo "  docker build -t $IMAGE_NAME ."
    exit 2
fi

# 執行 container，掛載 classbench_set
echo "[INFO] Launching Docker container with volume mounting..."
docker run --rm -it \
    -v "$(pwd)/../classbench_set:/workspace/classbench_set" \
    $IMAGE_NAME
