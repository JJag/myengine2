cmake_minimum_required(VERSION 4.2)
project(myengine2)

set(CMAKE_CXX_STANDARD 20)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "")

set(SDLSHADERCROSS_DXC OFF CACHE BOOL "")
set(SDLSHADERCROSS_CLI OFF CACHE BOOL "")
set(SDLSHADERCROSS_VENDORED ON CACHE BOOL "")
set(SDLSHADERCROSS_SPIRVCROSS_SHARED OFF CACHE BOOL "")

set(SDL_TEST_LIBRARY OFF CACHE BOOL "")
set(SPIRV_SKIP_TESTS ON CACHE BOOL "")

# dependencies
add_subdirectory(external/SDL)
add_subdirectory(external/SDL_shadercross)

add_executable(myengine2 src/main.cpp)

target_link_libraries(myengine2 PRIVATE
        SDL3::SDL3-static
        SDL3_shadercross::SDL3_shadercross-static
)

target_compile_options(myengine2 PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wsign-conversion
        -Wformat
        #        -Werror
)
