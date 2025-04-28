#!/bin/bash

## How to use
## chmod +x scripts/run_PT.sh
## sudo bash ./scripts/run_PT.sh

set -e
set -o pipefail

# ==============================================================================
# 變數設定區
# ==============================================================================

BUILD_DIR="./build"
EXECUTABLE="$BUILD_DIR/tests/PT_main"

RULESET_LIST=(
  acl1_100k
  acl2_100k
  acl3_100k
  acl4_100k
  # acl5_100k
  fw1_100k
  fw2_100k
  fw3_100k
  fw4_100k
  fw5_100k
  ipc1_100k
  #ipc2_100k
)

RULESET_DIR="../classbench_set/ipv4-ruleset"
TRACE_DIR="../classbench_set/ipv4-trace"

OUTPUT_DIR="$BUILD_DIR"
RUN_UPDATE_TEST=false  # 是否執行 update 測試，預設否

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

verify_input_files() {
    local ruleset_path="$1"
    local trace_path="$2"

    if [ ! -f "$ruleset_path" ]; then
        echo "[Error] Ruleset file not found: $ruleset_path"
        exit 1
    fi
    if [ ! -f "$trace_path" ]; then
        echo "[Error] Trace file not found: $trace_path"
        exit 1
    fi
}

setup_environment() {
    ulimit -s 81920
}

run_search_test() {
    local ruleset_name="$1"
    local ruleset_path="$2"
    local trace_path="$3"
    local short_name="${ruleset_name%%_*}"
    local output_file="${OUTPUT_DIR}/${EXECUTABLE##*/}_c_${short_name}.txt"

    echo "[Info] Running search test for: $ruleset_name"
    "$EXECUTABLE" -r "$ruleset_path" -p "$trace_path" > "$output_file"
    echo "[Info] Search output saved to: $output_file"
}

run_update_test() {
    local ruleset_name="$1"
    local ruleset_path="$2"
    local trace_path="$3"
    local short_name="${ruleset_name%%_*}"
    local output_file="${OUTPUT_DIR}/${EXECUTABLE##*/}_u_${short_name}.txt"

    echo "[Info] Running update test for: $ruleset_name"
    "$EXECUTABLE" -r "$ruleset_path" -p "$trace_path" -u > "$output_file"
    echo "[Info] Update output saved to: $output_file"
}

# ==============================================================================
# 主程式入口
# ==============================================================================

prepare_build_dir
build_project
setup_environment

for ruleset in "${RULESET_LIST[@]}"; do
    RULESET_PATH="$RULESET_DIR/$ruleset"
    TRACE_PATH="$TRACE_DIR/${ruleset}_trace"

    verify_input_files "$RULESET_PATH" "$TRACE_PATH"

    run_search_test "$ruleset" "$RULESET_PATH" "$TRACE_PATH"

    if [ "$RUN_UPDATE_TEST" = true ]; then
        run_update_test "$ruleset" "$RULESET_PATH" "$TRACE_PATH"
    fi
done

echo "[All Done] All rulesets tested successfully."
