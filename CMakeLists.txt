project(gnvim CXX)
cmake_minimum_required(VERSION 3.1)
set(CMAKE_CXX_STANDARD 11)

include(FindPkgConfig)

find_package(Git)
if(GIT_FOUND)
    execute_process(COMMAND git describe OUTPUT_VARIABLE GNVIM_GIT_VERSION
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE)
else()
    set(GNVIM_GIT_VERSION "")
endif()
add_definitions(-DGNVIM_GIT_VERSION=\"${GNVIM_GIT_VERSION}\"
    -DPACKAGE_VERSION=\"0.0.5\")

if(UNIX)
    add_definitions(-Wall -Wextra -Wunused)
endif()

pkg_check_modules(GNVIM REQUIRED gtkmm-3.0 msgpack)

include_directories(BEFORE src)

add_subdirectory(src)
