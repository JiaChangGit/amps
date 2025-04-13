# tests/CMakeLists.txt

# KSet_new  my_KSet
add_executable(KSet_new KSet_new.cpp)
target_link_libraries(KSet_new PRIVATE  my_KSet)


# PT_main 需要 my_PT
add_executable(PT_main PT_main.cpp)
target_link_libraries(PT_main PRIVATE my_PT)

# DBTable_main 需要 my_DBTable
add_executable(DBTable_main DBTable_main.cpp)
target_link_libraries(DBTable_main PRIVATE my_DBTable)


# main 需要 my_KSet my_PT my_DBTable Eigen3::Eigen
find_package(Eigen3 REQUIRED)
#find_package(dlib REQUIRED)
add_executable(main main.cpp)
target_link_libraries(main PRIVATE my_KSet my_PT my_DBTable Eigen3::Eigen )
