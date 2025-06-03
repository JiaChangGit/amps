#!/bin/bash

## How to use
## chmod +x scripts/per_run.sh
## bash ./scripts/per_run.sh
### Or
### sudo bash ./scripts/per_run.sh

# 設定變數
BUILD_DIR="./build"
TEST_MAIN_EXEC="$BUILD_DIR/tests/main"

# RULESET_PATH="../classbench_set/ipv4-ruleset/acl1_1k"
# TRACE_PATH="../classbench_set/ipv4-trace/acl1_1k_trace"

# RULESET_PATH="../classbench_set/ipv4-ruleset/acl1_100k"
# TRACE_PATH="../classbench_set/ipv4-trace/acl1_100k_trace"

# RULESET_PATH="../classbench_set/ipv4-ruleset/acl2_100k"
# TRACE_PATH="../classbench_set/ipv4-trace/acl2_100k_trace"

# RULESET_PATH="../classbench_set/ipv4-ruleset/acl3_100k"
# TRACE_PATH="../classbench_set/ipv4-trace/acl3_100k_trace"

# RULESET_PATH="../classbench_set/ipv4-ruleset/acl4_100k"
# TRACE_PATH="../classbench_set/ipv4-trace/acl4_100k_trace"

# RULESET_PATH="../classbench_set/ipv4-ruleset/acl5_100k"
# TRACE_PATH="../classbench_set/ipv4-trace/acl5_100k_trace"

# RULESET_PATH="../classbench_set/ipv4-ruleset/fw1_100k"
# TRACE_PATH="../classbench_set/ipv4-trace/fw1_100k_trace"

# RULESET_PATH="../classbench_set/ipv4-ruleset/fw2_100k"
# TRACE_PATH="../classbench_set/ipv4-trace/fw2_100k_trace"

# RULESET_PATH="../classbench_set/ipv4-ruleset/fw3_100k"
# TRACE_PATH="../classbench_set/ipv4-trace/fw3_100k_trace"

# RULESET_PATH="../classbench_set/ipv4-ruleset/fw4_100k"
# TRACE_PATH="../classbench_set/ipv4-trace/fw4_100k_trace"

# RULESET_PATH="../classbench_set/ipv4-ruleset/fw5_100k"
# TRACE_PATH="../classbench_set/ipv4-trace/fw5_100k_trace"

# RULESET_PATH="../classbench_set/ipv4-ruleset/ipc1_100k"
# TRACE_PATH="../classbench_set/ipv4-trace/ipc1_100k_trace"

RULESET_PATH="../classbench_set/ipv4-ruleset/ipc2_100k"
TRACE_PATH="../classbench_set/ipv4-trace/ipc2_100k_trace"

###
# TRACE_PATH="../classbench_set/ipv4-trace/part_0"

# 檢查並建立 build 目錄（如果不存在）
if [ ! -d "$BUILD_DIR" ]; then
    echo "Build directory not found. Creating one..."
    mkdir -p "$BUILD_DIR"
fi

# 檢查並執行 build.sh 來編譯
if [ ! -f "$TEST_MAIN_EXEC" ]; then
    echo "Test executable not found. Building the project..."
    chmod +x scripts/build.sh
    ./scripts/build.sh
fi

# 確保測試執行檔存在
if [ ! -f "$TEST_MAIN_EXEC" ]; then
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

echo "Running tests..."
"$TEST_MAIN_EXEC" -r "$RULESET_PATH" -p "$TRACE_PATH" -s >  "$TEST_MAIN_EXEC"_acl1.txt
# echo "Running Again..."
# "$TEST_MAIN_EXEC" -r "$RULESET_PATH" -p "$TRACE_PATH" -s >  "$TEST_MAIN_EXEC"_acl1.txt

# echo "Running tests..."
# "$TEST_MAIN_EXEC" -r "$RULESET_PATH" -p "$TRACE_PATH" -s >  "$TEST_MAIN_EXEC"_part0_ipc2.txt
# echo "Running Again..."
# "$TEST_MAIN_EXEC" -r "$RULESET_PATH" -p "$TRACE_PATH" -s >  "$TEST_MAIN_EXEC"_part0_ipc2.txt
