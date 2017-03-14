project(gnvim CXX)
cmake_minimum_required(VERSION 3.1)
set(CMAKE_CXX_STANDARD 11)

include(FindPkgConfig)

if(UNIX)
    add_definitions(-Wall -Wextra -Wunused)
endif()

pkg_check_modules(GNVIM REQUIRED gtkmm-3.0 msgpack)

include_directories(BEFORE src)

add_subdirectory(src)
