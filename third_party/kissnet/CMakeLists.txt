cmake_minimum_required(VERSION 3.13)

add_library(kissnet INTERFACE)

target_include_directories(kissnet INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
