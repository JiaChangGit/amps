# src/CMakeLists.txt

# 建立 my_KSet 函式庫，提供 KSet.cpp
add_library(my_KSet STATIC KSet.cpp)
# 設定 my_KSet 需要 include 目錄
target_include_directories(my_KSet PUBLIC ${PROJECT_SOURCE_DIR}/include/KSet)
target_include_directories(my_KSet PUBLIC ${PROJECT_SOURCE_DIR}/include)

###############
# 建立 my_PT 函式庫，提供 pt_tree.cpp
add_library(my_PT STATIC PT_tree.cpp)
# 設定 my_PT 需要 include 目錄
target_include_directories(my_PT PUBLIC ${PROJECT_SOURCE_DIR}/include/PT)
target_include_directories(my_PT PUBLIC ${PROJECT_SOURCE_DIR}/include)

# 建立 my_DBTable 函式庫，提供 DBTable.cpp
add_library(my_DBTable STATIC DBTable.cpp)
# 設定 my_DBTable 需要 include 目錄
target_include_directories(my_DBTable PUBLIC ${PROJECT_SOURCE_DIR}/include/DBT)
target_include_directories(my_DBTable PUBLIC ${PROJECT_SOURCE_DIR}/include)
###############


