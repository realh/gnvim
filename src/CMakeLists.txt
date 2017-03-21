set(GNVIM_SOURCES
    app.cpp
    main.cpp
    msgpack-adapter.cpp
    msgpack-rpc.cpp
    nvim-bridge.cpp
    view.cpp
    window.cpp
)

set(GNVIM_HEADERS
    app.h
    defns.h
    msgpack-adapter.h
    msgpack-rpc.h
    nvim-bridge.h
    view.h
    window.h
)

add_custom_command(OUTPUT nvim-bridge.cpp
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/nvim-signals.py reg ${CMAKE_CURRENT_SOURCE_DIR}/nvim-bridge.cpp.in ${CMAKE_CURRENT_BINARY_DIR}/nvim-bridge.cpp)
add_custom_command(OUTPUT nvim-bridge.h
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/nvim-signals.py def ${CMAKE_CURRENT_SOURCE_DIR}/nvim-bridge.h.in ${CMAKE_CURRENT_BINARY_DIR}/nvim-bridge.h)

add_executable(gnvim ${GNVIM_SOURCES} ${GNVIM_HEADERS})
target_include_directories(gnvim PUBLIC ${GNVIM_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})
target_compile_options(gnvim PUBLIC ${GNVIM_CFLAGS_OTHER})
target_link_libraries(gnvim PUBLIC ${GNVIM_LDFLAGS})
