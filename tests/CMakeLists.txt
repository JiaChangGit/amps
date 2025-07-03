# ./tests/CMakeLists.txt

find_package(Eigen3 REQUIRED)
find_package(pybind11 REQUIRED)
#find_package(dlib REQUIRED)

# 編譯 rl_gym 為 Python 模組
pybind11_add_module(rl_gym rl_gym.cpp)
target_include_directories(rl_gym PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/include/PT
    ${PROJECT_SOURCE_DIR}/include/DBT
    ${PROJECT_SOURCE_DIR}/include/DT
    ${PROJECT_SOURCE_DIR}/include/KSet
)
target_link_libraries(rl_gym PRIVATE my_PT my_DBTable my_DTMT Eigen3::Eigen)
# 為 rl_gym 添加特定的預處理器定義
# target_compile_definitions(rl_gym PRIVATE
    # TIMER_METHOD TIMER_RDTSCP
# )

set_target_properties(rl_gym PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/tests)

# main 需要 my_KSet my_PT my_DBTable my_DTMT Eigen3::Eigen
add_executable(main main.cpp)
target_include_directories(main PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/include/PT
    ${PROJECT_SOURCE_DIR}/include/DBT
    ${PROJECT_SOURCE_DIR}/include/DT
    ${PROJECT_SOURCE_DIR}/include/MT
    ${PROJECT_SOURCE_DIR}/include/KSet
)
target_link_libraries(main PRIVATE my_KSet my_PT my_DBTable my_DTMT Eigen3::Eigen)
# 為 main 添加特定的預處理器定義
# target_compile_definitions(main PRIVATE
    # TIMER_METHOD TIMER_STEADY_CLOCK
# )
