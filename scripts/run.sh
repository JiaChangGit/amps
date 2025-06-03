#!/bin/bash

## How to use
## chmod +x scripts/run.sh
## sudo bash ./scripts/run.sh

set -e
set -o pipefail

# ==============================================================================
# 變數設定區
# ==============================================================================
BUILD_DIR="./build"
EXECUTABLE="$BUILD_DIR/tests/main"

# 定義 ruleset 與 trace 名稱列表（不含路徑）
RULESET_NAMES=(
  acl1_100k acl2_100k acl3_100k acl4_100k acl5_100k
  fw1_100k fw2_100k fw3_100k fw4_100k fw5_100k
  ipc1_100k ipc2_100k
)

RULESET_DIR="../classbench_set/ipv4-ruleset"
TRACE_DIR="../classbench_set/ipv4-trace"

# 結果輸出資料夾（可選，這裡選擇直接輸出到 build/）
OUTPUT_DIR="$BUILD_DIR"

# ==============================================================================
# 功能函式區
# ==============================================================================

prepare_build_dir() {
    if [ ! -d "$BUILD_DIR" ]; then
        echo "[Info] Build directory not found. Creating..."
        mkdir -p "$BUILD_DIR"
    fi
}

build_project() {
    if [ ! -f "$EXECUTABLE" ]; then
        echo "[Info] Test executable not found. Building project..."
        chmod +x scripts/build.sh
        ./scripts/build.sh
    fi

    if [ ! -f "$EXECUTABLE" ]; then
        echo "[Error] Build failed. Test executable still missing."
        exit 1
    fi
}

setup_environment() {
    ulimit -s 81920
}

run_test_for_ruleset() {
    local ruleset_name="$1"
    local ruleset_path="$RULESET_DIR/$ruleset_name"
    local trace_path="$TRACE_DIR/${ruleset_name}_trace"
    #define VALID (main.cpp)
    # local trace_path="$TRACE_DIR/part_0"
    local output_file="$OUTPUT_DIR/${EXECUTABLE##*/}_c_${ruleset_name%%_*}.txt"

    # 檢查檔案存在
    if [ ! -f "$ruleset_path" ]; then
        echo "[Error] Ruleset file not found: $ruleset_path"
        exit 1
    fi
    if [ ! -f "$trace_path" ]; then
        echo "[Error] Trace file not found: $trace_path"
        exit 1
    fi

    echo "[Info] Running test for ruleset: ${ruleset_name}"
    "$EXECUTABLE" -r "$ruleset_path" -p "$trace_path" -s > "$output_file"
    echo "[Info] Output saved to: $output_file"
}

# ==============================================================================
# 主程式入口
# ==============================================================================

prepare_build_dir
build_project
setup_environment

for ruleset in "${RULESET_NAMES[@]}"; do
    run_test_for_ruleset "$ruleset"
done


echo "[All Done] All rulesets tested successfully."
