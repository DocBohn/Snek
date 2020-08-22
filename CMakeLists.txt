cmake_minimum_required(VERSION 3.13)
project(snek C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "-lcurses")

add_executable(snek main.c)