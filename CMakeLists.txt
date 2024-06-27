cmake_minimum_required(VERSION 3.24)
project(snek C)

set(CMAKE_C_STANDARD 99)

find_package(Curses REQUIRED)
include_directories(${CURSES_INCLUDE_DIR})

add_executable(snek main.c)
target_compile_options(snek PRIVATE -Wall -Wextra)
target_link_libraries(snek ${CURSES_LIBRARIES})