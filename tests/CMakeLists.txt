# tests/CMakeLists.txt

# KSet_main 需要 my_KSet
add_executable(KSet_main KSet_main.cpp)
target_link_libraries(KSet_main PRIVATE my_KSet)

# KSet_new 需要 input_lib 和 my_KSet
add_executable(KSet_new KSet_new.cpp)
target_link_libraries(KSet_new PRIVATE input_lib my_KSet)

# main 需要 input_lib
add_executable(main main.cpp)
target_link_libraries(main PRIVATE input_lib)
