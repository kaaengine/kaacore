cmake_minimum_required(VERSION 3.13)
cmake_policy(SET CMP0077 NEW)


add_executable(tests_runner runner.cpp basics.cpp)
target_link_libraries(tests_runner kaacore Catch2::Catch2)
set_target_properties(
    tests_runner PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)
