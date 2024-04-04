cmake_minimum_required(VERSION 3.13)
cmake_policy(SET CMP0077 NEW)

set(TEST_SRC_CXX_FILES
    test_basics.cpp
    test_shapes.cpp
    test_hitboxes.cpp
    test_textures.cpp
    test_materials.cpp
    test_statistics.cpp
    test_transitions.cpp
    test_draw_unit.cpp
    test_draw_queue.cpp
    test_geometry.cpp
    test_fonts.cpp
    test_unicode_buffer.cpp
)

add_executable(runner runner.cpp ${TEST_SRC_CXX_FILES})
target_link_libraries(runner kaacore Catch2::Catch2)
set_target_properties(
    runner PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)
