cmake_minimum_required(VERSION 3.12)

project(cell_automata)

add_subdirectory(raylib)
include_directories(raylib/src)

add_executable(cell_automata main.cpp)

set_property(TARGET cell_automata PROPERTY CXX_STANDARD 20)

target_link_libraries(cell_automata raylib)
