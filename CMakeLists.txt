cmake_minimum_required(VERSION 3.10)
project(MyProject)

# 設定 C++ 標準
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # 確保使用標準 C++，不使用 GCC 擴展

# 啟用 OpenMP
find_package(OpenMP REQUIRED)
if (OPENMP_FOUND)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif()

# 通用編譯選項
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive -pedantic -Wall -Wextra -mbmi2")

# 添加 src 和 tests 子目錄（確保 src 在前，tests 依賴 src）
add_subdirectory(src)
add_subdirectory(tests)

# 啟用最佳化選項
option(ENABLE_OPTIMIZATION "Enable high-performance optimizations" ON)
if (ENABLE_OPTIMIZATION)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        message(STATUS "Using GCC/G++ Compiler - Enabling Optimizations")

        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} \
            -Ofast -funroll-loops -fno-stack-protector -ffast-math -finline-functions \
            -fwhole-program -fcse-skip-blocks -funsafe-loop-optimizations")

        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} \
            -march=native -mtune=native -mf16c -mmmx -msse -msse2 -msse3 \
            -mssse3 -msse4 -msse4.1 -msse4.2 -mpopcnt -mabm -mavx")
    endif()
endif()


# 顯示最終的編譯選項（用於 Debug）
message(STATUS "Final CXX Flags: ${CMAKE_CXX_FLAGS}")
