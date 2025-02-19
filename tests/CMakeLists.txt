# 設定測試執行檔
set(TEST_FILES main.cpp)

# 建立測試可執行檔
add_executable(my_project_tests ${TEST_FILES})

# 連結 src 內的函式庫
target_link_libraries(my_project_tests my_project_lib)
