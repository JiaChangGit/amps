# tests/CMakeLists.txt

# main 需要 my_KSet my_PT my_DBTable Eigen3::Eigen
find_package(Eigen3 REQUIRED)
#find_package(dlib REQUIRED)
add_executable(main main.cpp)
target_link_libraries(main PRIVATE my_KSet my_PT my_DBTable Eigen3::Eigen )
