# ./
# 設定 CMake 最低版本要求
cmake_minimum_required(VERSION 3.12)

# 定義專案名稱
project(MyProject)

# ==============================================================================
# 設定 C++ 語言版本為 C++17，並強制開啟此設定
# - `CMAKE_CXX_STANDARD_REQUIRED`：要求使用指定的 C++ 標準（否則報錯）
# - `CMAKE_CXX_EXTENSIONS OFF`：禁用 compiler-specific extensions（例如 GNU extensions）
# ==============================================================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


# # 尋找並啟用 OpenMP
# find_package(OpenMP REQUIRED)
# if (OPENMP_FOUND)
#     set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
#     set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
# else()
#     message(FATAL_ERROR "OpenMP not found. Please install it (e.g., sudo apt install libomp-dev)")
# endif()

# ==============================================================================
# 尋找 Eigen3 數學函式庫（用於線性代數、矩陣運算）
# Ubuntu 預設安裝路徑為 /usr/include/eigen3，NO_MODULE 表示不使用 Module 方式
# 若找不到則中止建置，並顯示錯誤提示（建議安裝命令）
# ==============================================================================
find_package(Eigen3 3.3 REQUIRED NO_MODULE)
if (EIGEN3_FOUND)
    message(STATUS "Eigen3 found: ${EIGEN3_INCLUDE_DIR}")
else()
    message(FATAL_ERROR "Eigen3 not found. Please install it (e.g., sudo apt install libeigen3-dev)")
endif()


# ==============================================================================
# 基本編譯旗標（所有 build type 都適用）
# -pedantic       ：強制標準相容性（例如禁止非標準語法）
# -ffast-math     ：允許編譯器進行更積極的數學運算優化
# -freciprocal-math:對 division 使用近似乘法反轉，適合數值迴圈但不保證精度
# -Wall           ：開啟所有基本警告
# -Wextra         ：更多額外警告（例如未使用的參數）
# -Wreturn-local-addr：警告不必要的 move
# -fopt-info-optimized:loop unrolling disabled ... or ...
# -fomit-frame-pointer:移除 frame pointer，釋出額外暫存器（GCC 在非 Debug 模式多已預設）
# -fprefetch-loop-arrays:自動加入 prefetch 指令，對大型陣列操作可能提升效能
# -ftree-vectorize:強制開啟 GCC 向量化（通常已由 -O3 開啟）
# -fno-semantic-interposition:對大型專案可減少符號查找開銷（會限制 dynamic linking 彈性）
# -falign-functions=32 OR 64:函數對齊，可改善指令快取與預取效果（值依硬體調整）
# -falign-loops=32 OR 64:用來控制迴圈（loop）在組譯後的對齊位元組數，單位為 Byte
# ==============================================================================
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic -ffast-math -freciprocal-math")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -Wall -Wextra -Wreturn-local-addr -fopt-info-optimized")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fomit-frame-pointer -fprefetch-loop-arrays -ftree-vectorize -fno-semantic-interposition -falign-functions=32 -falign-loops=32")

# ==============================================================================
# 根據編譯器選擇啟用最佳化參數（GCC / Clang）
# 包含：
# - `-O3`：最高等級優化
# - `-march=native` + `-mtune=native`：針對本地 CPU 架構做優化
# - `-mbmi2`：啟用 Intel BMI2 指令集（加速 bit manipulation）
# - `-funroll-loops`：展開迴圈增加 pipeline 效率
# - `-finline-functions`：允許跨檔案函數內聯
# - `-fassociative-math`：更寬鬆的數學重排自由度（注意：非 IEEE 精確）
# - `-flto`：Link-time Optimization，跨模組優化
# - `-fipa-pta`：會在 LTO 階段嘗試跨函式分析資料指向，對 inline decision、alias analysis、function pruning 有影響
# - `-fprofile-use`：選擇最佳化路徑（hot/cold block 重排序），判斷哪些函數最適合 inline 或 optimize for size
# 如果有啟用 OpenMP（見下方 find_package），則加入 -fopenmp 支援平行運算
# ==============================================================================
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    message(STATUS "Using GCC/G++ Compiler - Enabling Optimizations with BMI2")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native -mtune=native -mbmi2 -fassociative-math")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -funroll-loops -finline-functions -flto -fipa-pta -fprofile-use")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -flto -fipa-pta -fprofile-use")

elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    message(STATUS "Using Clang Compiler - Enabling Optimizations with BMI2")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native -mtune=native -mbmi2 -fassociative-math")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -funroll-loops -flto -fipa-pta -fprofile-use")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -flto -fipa-pta -fprofile-use")

else()
    message(WARNING "Unsupported compiler. Optimizations may not be fully applied.")
endif()
# 注意：-funroll-all-loops 比 -funroll-loops 更激進，容易導致 binary 膨脹
# -fno-unroll-loops 禁止所有自動 loop unrolling（包含 -O3 、 -funroll-loops 和 -funroll-all-loops）
# -funroll-loops  VS  -funroll-all-loops  VS  -fno-unroll-loops

# ==============================================================================
# 關閉 assert 斷言與 Debug 模式，進一步提升效能
# -DNDEBUG：表示定義「非除錯模式」，`assert()` 巨集會被移除
# ==============================================================================
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNDEBUG")


# ==============================================================================
# 新增子目錄編譯
# 注意：tests 通常會依賴 src，所以順序很重要
# - 可選：加入第三方 dlib 支援
# ==============================================================================
# add_subdirectory(third_party/dlib)  # 可視需求啟用
add_subdirectory(src)
add_subdirectory(tests)


# ==============================================================================
# 最後輸出編譯選項與連結選項（方便除錯與確認設定）
# ==============================================================================
message(STATUS "Final CXX Flags: ${CMAKE_CXX_FLAGS}")
message(STATUS "Final Linker Flags: ${CMAKE_EXE_LINKER_FLAGS}")

