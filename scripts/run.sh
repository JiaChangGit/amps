#!/bin/bash

## HOW to use
# chmod +x ./scripts/run.sh
# bash ./scripts/run.sh
# Or
# bash ./scripts/run.sh

set -e
set -o pipefail

# ==============================================================================
# 變數設定區
# ==============================================================================
BUILD_DIR="./build"
EXECUTABLE="$BUILD_DIR/tests/main"
TIMESTAMP=$(date +"%m%d_%H%M%S")
# 定義規則集名稱列表（不含路徑）
# RULESET_NAMES=(
   # acl1_1k acl2_1k acl3_1k acl4_1k acl5_1k
  # fw1_1k fw2_1k fw3_1k fw4_1k fw5_1k ipc1_1k ipc2_1k
  # acl1_10k acl2_10k acl3_10k acl4_10k acl5_10k fw1_10k fw2_10k
  # fw3_10k fw4_10k fw5_10k ipc1_10k ipc2_10k
    # acl1_5k acl2_5k acl3_5k
  # acl4_5k acl5_5k fw1_5k fw2_5k fw3_5k
  # fw4_5k fw5_5k ipc1_5k ipc2_5k
# )
#RULESET_NAMES=(
  #acl1_5k acl2_5k acl3_5k
  #acl4_5k acl5_5k fw1_5k fw2_5k fw3_5k
  #fw4_5k fw5_5k ipc1_5k ipc2_5k
#)
#RULESET_NAMES=(
  #acl1_10k acl2_10k acl3_10k acl4_10k acl5_10k fw1_10k fw2_10k
  #fw3_10k fw4_10k fw5_10k ipc1_10k ipc2_10k
#)
RULESET_NAMES=(
     acl1_1k acl2_1k acl3_1k acl4_1k acl5_1k
  fw1_1k fw2_1k fw3_1k fw4_1k fw5_1k ipc1_1k ipc2_1k
  acl1_10k acl2_10k acl3_10k acl4_10k acl5_10k fw1_10k fw2_10k
  fw3_10k fw4_10k fw5_10k ipc1_10k ipc2_10k
    acl1_5k acl2_5k acl3_5k
  acl4_5k acl5_5k fw1_5k fw2_5k fw3_5k
  fw4_5k fw5_5k ipc1_5k ipc2_5k
  acl1_100k acl2_100k
  acl3_100k
  acl4_100k
  acl5_100k
  fw1_100k
  fw2_100k
  fw3_100k
  fw4_100k fw5_100k ipc1_100k
  ipc2_100k
)
RULESET_DIR="../classbench_set/ipv4-ruleset"
TRACE_DIR="../classbench_set/ipv4-trace"

# 結果輸出根目錄
OUTPUT_DIR="$BUILD_DIR"

# ==============================================================================
# 功能函式區
# ==============================================================================

prepare_build_dir() {
    # 檢查並創建 build 目錄
    if [ ! -d "$BUILD_DIR" ]; then
        echo "[Info] Build directory not found. Creating..."
        mkdir -p "$BUILD_DIR"
    fi
}

build_project() {
    # 檢查測試可執行檔是否存在，若不存在則執行建置
    if [ ! -f "$EXECUTABLE" ]; then
        echo "[Info] Test executable not found. Building project..."
        chmod +x scripts/build.sh
        ./scripts/build.sh
    fi

    # 再次確認可執行檔是否存在
    if [ ! -f "$EXECUTABLE" ]; then
        echo "[Error] Build failed. Test executable still missing."
        exit 1
    fi
}

setup_environment() {
    # 設置環境參數（堆疊大小）
    ulimit -s 81920
}

run_test_for_ruleset() {
    local ruleset_name="$1"
    local ruleset_path="$RULESET_DIR/$ruleset_name"
    local trace_path="$TRACE_DIR/${ruleset_name}_trace"
    # check part_0
    #local trace_path="$TRACE_DIR/part_0"
    local output_file="$OUTPUT_DIR/${EXECUTABLE##*/}_c_${ruleset_name%%_*}.txt"
    local result_dir="./Results/$TIMESTAMP/$ruleset_name"

    # 檢查規則集和追蹤檔案是否存在
    if [ ! -f "$ruleset_path" ]; then
        echo "[Error] Ruleset file not found: $ruleset_path"
        exit 1
    fi
    if [ ! -f "$trace_path" ]; then
        echo "[Error] Trace file not found: $trace_path"
        exit 1
    fi

    # 執行強化學習訓練
    echo "[param] Running RL: ${ruleset_name}"
    python3 ./tests/train.py --rules "$ruleset_path" --trace "$trace_path"

    # 執行測試程式並儲存輸出
    echo "[Info] Running test for ruleset: ${ruleset_name}"
    "$EXECUTABLE" -r "$ruleset_path" -p "$trace_path" -s > "$output_file"

    # 創建專屬結果資料夾
    mkdir -p "$result_dir"

    # 複製 *.txt 並加上規則集名稱前綴
    if [ -f "./INFO/param.txt" ]; then
        echo "[CP] Copy ./INFO/*.txt to ${result_dir} ..."
        cp ./INFO/*.txt "${result_dir}"
    else
        echo "[Warning] param.txt not found in ./INFO/"
    fi

    # 複製測試輸出檔案
    echo "[CP] Copy $output_file ..."
    cp "$output_file" "${result_dir}/${EXECUTABLE##*/}_c_${ruleset_name}.txt"

    echo "[Info] Output saved to: $output_file and $result_dir"
}

# ==============================================================================
# 主程式入口
# ==============================================================================

prepare_build_dir
build_project
setup_environment

# 遍歷所有規則集並執行測試
for ruleset in "${RULESET_NAMES[@]}"; do
    run_test_for_ruleset "$ruleset"
done

echo "[All Done] All rulesets tested successfully."
