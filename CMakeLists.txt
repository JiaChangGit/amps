cmake_minimum_required(VERSION 3.10)
project(MyProject)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 包含目錄
include_directories(include)

# 添加子目錄
add_subdirectory(src)
add_subdirectory(tests)
