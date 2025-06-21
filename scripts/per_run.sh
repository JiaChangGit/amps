#!/bin/bash

## How to use
## bash ./scripts/per_run.sh

# acl5 cannot with part_0
# ipc2 cannot with part_0

set -e
set -o pipefail

BUILD_DIR="./build"
TEST_MAIN_EXEC="$BUILD_DIR/tests/main"
TIMESTAMP=$(date +"%m%d_%H%M%S")
# ✅ 單一資料集（可切換）
# RULESET_PATH="../classbench_set/ipv4-ruleset/acl1_1k"
# TRACE_PATH="../classbench_set/ipv4-trace/acl1_1k_trace"

# RULESET_PATH="../classbench_set/ipv4-ruleset/acl1_100k"
# TRACE_PATH="../classbench_set/ipv4-trace/acl1_100k_trace"

RULESET_PATH="../classbench_set/ipv4-ruleset/acl2_100k"
TRACE_PATH="../classbench_set/ipv4-trace/acl2_100k_trace"

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

# RULESET_PATH="../classbench_set/ipv4-ruleset/ipc2_100k"
# TRACE_PATH="../classbench_set/ipv4-trace/ipc2_100k_trace"

###
# TRACE_PATH="../classbench_set/ipv4-trace/part_0"

# 檢查與建構
if [ ! -d "$BUILD_DIR" ]; then
    echo "Build directory not found. Creating..."
    mkdir -p "$BUILD_DIR"
fi

if [ ! -f "$TEST_MAIN_EXEC" ]; then
    echo "Test executable not found. Building project..."
    chmod +x scripts/build.sh
    ./scripts/build.sh
fi

if [ ! -f "$RULESET_PATH" ] || [ ! -f "$TRACE_PATH" ]; then
    echo "Error: Ruleset or trace file not found!"
    exit 1
fi

RULESET_BASENAME=$(basename "$RULESET_PATH")  # e.g., acl2_100k
OUTPUT_FILE="${TEST_MAIN_EXEC}_${RULESET_BASENAME}.txt"

# 訓練與推論
ulimit -s 81920
echo "[param] Running RL: ${RULESET_BASENAME}"
python3 ./tests/train.py --rules "$RULESET_PATH" --trace "$TRACE_PATH"

# 執行分類器測試
echo "[run] Running C++ classifier on $RULESET_BASENAME ..."
"$TEST_MAIN_EXEC" -r "$RULESET_PATH" -p "$TRACE_PATH" -s > "$OUTPUT_FILE"

mkdir -p "./Results/$TIMESTAMP"
echo "[CP] Copy ./INFO/*.txt ..."
cp ./INFO/*.txt "./Results/$TIMESTAMP"

echo "[CP] Copy $OUTPUT_FILE ..."
cp "$OUTPUT_FILE" "./Results/$TIMESTAMP"

echo "[done] Output saved to: $OUTPUT_FILE and ./Results/$TIMESTAMP"
