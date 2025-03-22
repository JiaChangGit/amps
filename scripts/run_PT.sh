#!/bin/bash

## How to use
## chmod +x scripts/run_test.sh
## ./scripts/run_test.sh
### Or
### sudo sh ./scripts/run_test.sh

# 設定變數
BUILD_DIR="./build"
#############
TEST_KSET_MAIN_EXEC="$BUILD_DIR/tests/PT_main"
#############
RULESET_PATH="../classbench_set/ipv4-ruleset/acl1_100k"
TRACE_PATH="../classbench_set/ipv4-trace/acl1_100k_trace"

# 檢查並建立 build 目錄（如果不存在）
if [ ! -d "$BUILD_DIR" ]; then
    echo "Build directory not found. Creating one..."
    mkdir -p "$BUILD_DIR"
fi

# 檢查並執行 build.sh 來編譯
if [ ! -f "$TEST_KSET_MAIN_EXEC" ]; then
    echo "Test executable not found. Building the project..."
    chmod +x scripts/build.sh
    ./scripts/build.sh
fi

# 確保測試執行檔存在
if [ ! -f "$TEST_KSET_MAIN_EXEC" ]; then
    echo "Error: Test executable still not found after build. Exiting."
    exit 1
fi

# 確保規則集與測試資料存在
if [ ! -f "$RULESET_PATH" ] || [ ! -f "$TRACE_PATH" ]; then
    echo "Error: Ruleset or trace file not found!"
    exit 1
fi

# 執行測試
ulimit -s 81920

echo "Running search tests..."
"$TEST_KSET_MAIN_EXEC" -r "$RULESET_PATH" -p "$TRACE_PATH" > "$TEST_KSET_MAIN_EXEC"_c_log.txt

echo "Running upd tests..."
"$TEST_KSET_MAIN_EXEC" -r "$RULESET_PATH" -p "$TRACE_PATH" -u > "$TEST_KSET_MAIN_EXEC"_u_log.txt
