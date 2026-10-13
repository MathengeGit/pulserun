cmake_minimum_required(VERSION 3.10)
project(PulseRun CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(pulse_core src/core/framebuffer.cpp)
target_include_directories(pulse_core PUBLIC src)

add_executable(demo_setup demos/demo_setup.cpp)
target_link_libraries(demo_setup pulse_core)