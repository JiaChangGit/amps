# ./
cmake_minimum_required(VERSION 3.12)
project(MyProject)

# 設定 C++ 標準
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)  # 使用標準 C++，不啟用 GCC 擴展

# 尋找並啟用 OpenMP
find_package(OpenMP REQUIRED)
if (OPENMP_FOUND)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
else()
    message(FATAL_ERROR "OpenMP not found. Please install it (e.g., sudo apt install libomp-dev)")
endif()

# 尋找 Eigen 庫（Ubuntu 22.04 預設路徑為 /usr/include/eigen3）
find_package(Eigen3 3.3 REQUIRED NO_MODULE)
if (EIGEN3_FOUND)
    message(STATUS "Eigen3 found: ${EIGEN3_INCLUDE_DIR}")
else()
    message(FATAL_ERROR "Eigen3 not found. Please install it (e.g., sudo apt install libeigen3-dev)")
endif()

# 通用編譯選項
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")

# 最佳化選項（明確支持 BMI2）
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    message(STATUS "Using GCC/G++ Compiler - Enabling Optimizations with BMI2")
    # 確保使用 native 架構並明確啟用 BMI2
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native -mtune=native -mbmi2 -fopenmp")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -funroll-loops -finline-functions -flto")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -flto")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    message(STATUS "Using Clang Compiler - Enabling Optimizations with BMI2")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native -mtune=native -mbmi2")
    if (OPENMP_FOUND)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp")
    endif()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -funroll-loops -flto")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -flto")
else()
    message(WARNING "Unsupported compiler. Optimizations may not be fully applied.")
endif()

# 設定高效能模式
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNDEBUG")  # 禁用斷言，提升效能

# 添加 src 和 tests 子目錄（確保third_party/dlib在前，以及src 在前，tests 依賴 src）
#add_subdirectory(third_party/dlib)
add_subdirectory(src)
add_subdirectory(tests)

# 顯示最終編譯選項
message(STATUS "Final CXX Flags: ${CMAKE_CXX_FLAGS}")
message(STATUS "Final Linker Flags: ${CMAKE_EXE_LINKER_FLAGS}")
