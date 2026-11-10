cmake_minimum_required(VERSION 3.10)
project(PulseRun CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# The software renderer is ~10x slower unoptimised, so default to an optimised build.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# PULSERUN_HEADLESS=ON skips SDL2: the game then renders PNG files instead of opening a window.
option(PULSERUN_HEADLESS "Build without a window (PNG output only)" OFF)

if(MSVC)
  add_compile_options(/W3)
else()
  add_compile_options(-Wall -Wextra)
endif()

# Every .cpp under src/ goes into one static library: pulse_core.
file(GLOB_RECURSE PULSE_SOURCES CONFIGURE_DEPENDS src/*.cpp)
add_library(pulse_core ${PULSE_SOURCES})
target_include_directories(pulse_core PUBLIC src)

# Window backend: Emscripten (browser canvas), SDL2 (desktop) or headless. See src/core/window.h.
if(EMSCRIPTEN)
  set(PULSERUN_BACKEND "web")
elseif(NOT PULSERUN_HEADLESS)
  find_package(SDL2 CONFIG QUIET)
  if(SDL2_FOUND)
    set(PULSERUN_BACKEND "sdl2")
    target_compile_definitions(pulse_core PUBLIC PULSERUN_SDL)
    target_link_libraries(pulse_core PUBLIC SDL2::SDL2)
  else()
    set(PULSERUN_BACKEND "headless")
    message(STATUS "SDL2 not found: building the headless (PNG-only) game")
  endif()
else()
  set(PULSERUN_BACKEND "headless")
endif()
message(STATUS "PulseRun window backend: ${PULSERUN_BACKEND}")

# The game itself.
add_executable(pulserun app/main.cpp)
target_link_libraries(pulserun pulse_core)
target_compile_definitions(pulserun PRIVATE PULSERUN_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
if(EMSCRIPTEN)
  # pulserun.js + pulserun.wasm, loaded by web/index.html. Level files are embedded.
  target_link_options(pulserun PRIVATE -sALLOW_MEMORY_GROWTH=1 -sENVIRONMENT=web
                      "--embed-file=${CMAKE_SOURCE_DIR}/levels@/levels")
endif()

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
