#!/bin/bash

## How to use
## chmod +x scripts/build.sh
## bash ./scripts/build.sh
### Or
### bash ./scripts/build.sh

# 設定變數
BUILD_DIR="./build"

# 建立 build 目錄（如果不存在）
if [ ! -d "$BUILD_DIR" ]; then
    mkdir "$BUILD_DIR"
fi

# 進入 build 目錄
cd "$BUILD_DIR" || exit

# 執行 CMake 產生 Makefile
cmake ..

# 執行 make 進行編譯
make -j$(nproc)
