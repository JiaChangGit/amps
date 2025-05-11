# ./src/CMakeLists.txt

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


# =================== DT + MT 組合函式庫 =====================

# 收集所有 DT/MT 原始碼
file(GLOB_RECURSE DT_MT_SOURCES
    ${PROJECT_SOURCE_DIR}/src/DTsrc/*.cpp
    ${PROJECT_SOURCE_DIR}/src/MTsrc/*.cpp
    ${PROJECT_SOURCE_DIR}/src/elementary_DT_MT.cpp
)

# 加入 DT_MT 函式庫
add_library(my_DTMT STATIC ${DT_MT_SOURCES})

# 包含所有對應 header 路徑
target_include_directories(my_DTMT PUBLIC
    ${PROJECT_SOURCE_DIR}/include/DT
    ${PROJECT_SOURCE_DIR}/include/MT
    ${PROJECT_SOURCE_DIR}/include
)
