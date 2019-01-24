cmake_minimum_required(VERSION 3.13)
cmake_policy(SET CMP0077 NEW)

add_executable(demo-basic basic.cpp)
list(APPEND DEMO_TARGETS demo-basic)


target_link_libraries(${DEMO_TARGETS} kaacore)
set_target_properties(
    ${DEMO_TARGETS}
    PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)
