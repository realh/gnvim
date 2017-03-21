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

add_executable(gnvim ${GNVIM_SOURCES})
target_include_directories(gnvim PUBLIC ${GNVIM_INCLUDE_DIRS})
target_compile_options(gnvim PUBLIC ${GNVIM_CFLAGS_OTHER})
target_link_libraries(gnvim PUBLIC ${GNVIM_LDFLAGS})
