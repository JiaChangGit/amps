# 設定測試執行檔
set(TEST_FILES main.cpp)

# 建立測試可執行檔
add_executable(main ${TEST_FILES})

# 連結 src 內的函式庫
target_link_libraries(main my_project_lib)
