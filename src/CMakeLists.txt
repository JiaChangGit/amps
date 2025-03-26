# src/CMakeLists.txt

# 建立 my_KSet 函式庫，提供 KSet.cpp
add_library(my_KSet STATIC KSet.cpp)
# 設定 my_KSet 需要 include 目錄
target_include_directories(my_KSet PUBLIC ${PROJECT_SOURCE_DIR}/include)

###############
# 建立 my_PT 函式庫，提供 pt_tree.cpp
add_library(my_PT STATIC pt_tree.cpp)
# 設定 my_PT 需要 include 目錄
target_include_directories(my_PT PUBLIC ${PROJECT_SOURCE_DIR}/include/PT)
target_include_directories(my_PT PUBLIC ${PROJECT_SOURCE_DIR}/include)

# 建立 my_DBTable 函式庫，提供 DBTable.cpp
add_library(my_DBTable STATIC DBTable.cpp)
# 設定 my_DBTable 需要 include 目錄
target_include_directories(my_DBTable PUBLIC ${PROJECT_SOURCE_DIR}/include/DBT)
target_include_directories(my_DBTable PUBLIC ${PROJECT_SOURCE_DIR}/include)
###############

# 建立 input_lib 函式庫，包含 input.cpp 和 inputFile_test.cpp
add_library(input_lib STATIC input.cpp inputFile_test.cpp)
# 設定 input_lib 需要 include 目錄
target_include_directories(input_lib PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_include_directories(input_lib PUBLIC ${PROJECT_SOURCE_DIR}/third_party/dlib)
