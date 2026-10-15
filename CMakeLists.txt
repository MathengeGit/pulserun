cmake_minimum_required(VERSION 3.10)
project(PulseRun CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
  add_compile_options(/W3)
else()
  add_compile_options(-Wall -Wextra)
endif()

# Every .cpp under src/ goes into one static library: pulse_core.
file(GLOB_RECURSE PULSE_SOURCES CONFIGURE_DEPENDS src/*.cpp)
add_library(pulse_core ${PULSE_SOURCES})
target_include_directories(pulse_core PUBLIC src)

# One demo executable per graphics technique, plus the setup smoke test.
set(DEMOS setup lines circles polygons clipping curves scenes)
foreach(demo ${DEMOS})
  add_executable(demo_${demo} demos/demo_${demo}.cpp)
  target_link_libraries(demo_${demo} pulse_core)
endforeach()

# Automated checks for the algorithms:  ctest --test-dir build   (or run build/test_algorithms)
enable_testing()
add_executable(test_algorithms tests/test_algorithms.cpp)
target_link_libraries(test_algorithms pulse_core)
add_test(NAME algorithms COMMAND test_algorithms)

# `cmake --build build --target screenshots` renders every demo image into docs/screenshots/.
add_custom_target(screenshots
  COMMAND demo_lines     "${CMAKE_SOURCE_DIR}/docs/screenshots"
  COMMAND demo_circles   "${CMAKE_SOURCE_DIR}/docs/screenshots"
  COMMAND demo_polygons  "${CMAKE_SOURCE_DIR}/docs/screenshots"
  COMMAND demo_clipping  "${CMAKE_SOURCE_DIR}/docs/screenshots"
  COMMAND demo_curves    "${CMAKE_SOURCE_DIR}/docs/screenshots"
  COMMAND demo_scenes    "${CMAKE_SOURCE_DIR}/docs/screenshots"
  DEPENDS demo_lines demo_circles demo_polygons demo_clipping demo_curves demo_scenes
  COMMENT "Rendering all PulseRun demo screenshots"
)
