set(GNVIM_SOURCES
    app.cpp
    main.cpp
    msgpack-rpc.cpp
    view.cpp
    window.cpp
)

set(GNVIM_HEADERS
    app.h
    defns.h
    msgpack-rpc.h
    view.h
    window.h
)

add_executable(gnvim ${GNVIM_SOURCES})
target_include_directories(gnvim PUBLIC ${GNVIM_INCLUDE_DIRS})
target_compile_options(gnvim PUBLIC ${GNVIM_CFLAGS_OTHER} -pthread)
target_link_libraries(gnvim PUBLIC ${GNVIM_LDFLAGS} pthread)
