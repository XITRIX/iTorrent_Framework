cmake_minimum_required(VERSION 3.10)
include(submodules/cmake/toolchain.cmake)

project(LibTorrent)
set(BUILD_SHARED_LIBS OFF)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/submodules/libtorrent)