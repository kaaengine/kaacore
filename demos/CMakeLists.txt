cmake_minimum_required(VERSION 3.13)
cmake_policy(SET CMP0077 NEW)

function(_add_demo_target TARGET_NAME SOURCE_FILE)
    add_executable(${TARGET_NAME} ${SOURCE_FILE})
    target_link_libraries(${TARGET_NAME} kaacore)

    set_target_properties(
        ${TARGET_NAME} PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
    )
endfunction()

function(add_demo TARGET_NAME SOURCE_FILE)
    _add_demo_target(${TARGET_NAME} ${SOURCE_FILE})

    if (KAACORE_BUILD_DEMOS_WITH_TSAN)
        _add_demo_target(${TARGET_NAME}-tsan ${SOURCE_FILE})
        target_compile_options(${TARGET_NAME}-tsan PRIVATE -fsanitize=thread)
        target_link_libraries(${TARGET_NAME}-tsan -ltsan)
    endif()
endfunction()

add_demo(demo-basic-scene basic_scene.cpp)
add_demo(demo-physics physics.cpp)
add_demo(demo-sprites sprites.cpp)
add_demo(demo-polygon-tester polygon_tester.cpp)
add_demo(demo-fonts fonts.cpp)
add_demo(demo-timers timers.cpp)
add_demo(demo-transitions transitions.cpp)
add_demo(demo-controllers controllers.cpp)
add_demo(demo-window window.cpp)
