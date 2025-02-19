# 設定源文件
set(SRC_FILES input.cpp inputFile_test.cpp)

# 建立靜態函式庫
add_library(my_project_lib ${SRC_FILES})

# 設定 include 目錄
target_include_directories(my_project_lib PUBLIC ../include/my_project)
