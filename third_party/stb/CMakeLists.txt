cmake_minimum_required(VERSION 3.13)

set(STB_IMPL_FILE stb_impl.c)
set(STB_H_FILES stb_rect_pack.h stb_truetype.h)

add_library(stb OBJECT ${STB_IMPL_FILE} ${STB_H_FILES})

target_include_directories(stb PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
