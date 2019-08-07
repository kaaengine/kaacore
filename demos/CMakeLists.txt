cmake_minimum_required(VERSION 3.13)
cmake_policy(SET CMP0077 NEW)

add_executable(demo-basic basic.cpp)
list(APPEND DEMO_TARGETS demo-basic)

add_executable(demo-basic-scene basic_scene.cpp)
list(APPEND DEMO_TARGETS demo-basic-scene)

add_executable(demo-physics physics.cpp)
list(APPEND DEMO_TARGETS demo-physics)

add_executable(demo-sprites sprites.cpp)
list(APPEND DEMO_TARGETS demo-sprites)

add_executable(demo-polygon-tester polygon_tester.cpp)
list(APPEND DEMO_TARGETS demo-polygon-tester)

add_executable(demo-fonts fonts.cpp)
list(APPEND DEMO_TARGETS demo-fonts)

add_executable(demo-timers timers.cpp)
list(APPEND DEMO_TARGETS demo-timers)

add_executable(demo-transitions transitions.cpp)
list(APPEND DEMO_TARGETS demo-transitions)

target_link_libraries(demo-basic kaacore)
target_link_libraries(demo-basic-scene kaacore)
target_link_libraries(demo-physics kaacore)
target_link_libraries(demo-sprites kaacore)
target_link_libraries(demo-polygon-tester kaacore)
target_link_libraries(demo-fonts kaacore)
target_link_libraries(demo-timers kaacore)
target_link_libraries(demo-transitions kaacore)
set_target_properties(
    ${DEMO_TARGETS}
    PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)
