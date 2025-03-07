# src/CMakeLists.txt

# 建立 my_KSet 函式庫，提供 KSet.cpp
add_library(my_KSet STATIC KSet.cpp)
# 設定 my_KSet 需要 include 目錄
target_include_directories(my_KSet PUBLIC ${PROJECT_SOURCE_DIR}/include)

# 建立 input_lib 函式庫，包含 input.cpp 和 inputFile_test.cpp
add_library(input_lib STATIC input.cpp inputFile_test.cpp)
# 設定 input_lib 需要 include 目錄
target_include_directories(input_lib PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_include_directories(my_KSet PUBLIC ${PROJECT_SOURCE_DIR}/include)
