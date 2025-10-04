cmake_minimum_required(VERSION 3.10)
project(lv_keyboard_t9 C)

# LVGL include path (adjust as needed)
set(LVGL_DIR "${CMAKE_SOURCE_DIR}/lvgl")
include_directories(${LVGL_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(lv_keyboard_t9 STATIC
    src/lv_keyboard_t9.c
)

target_include_directories(lv_keyboard_t9 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${LVGL_DIR}
)

target_link_libraries(lv_keyboard_t9 PRIVATE lvgl)

# Usage:
# add_subdirectory(lv_keyboard_t9)
# target_link_libraries(your_app PRIVATE lv_keyboard_t9)
