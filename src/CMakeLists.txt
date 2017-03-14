set(GNVIM_SOURCES
    app.cpp
    main.cpp
    window.cpp
)

set(GNVIM_HEADERS
    app.h
    defns.h
    window.h
)

add_executable(gnvim ${GNVIM_SOURCES})
target_include_directories(gnvim PUBLIC ${GNVIM_INCLUDE_DIRS})
target_compile_options(gnvim PUBLIC ${GNVIM_CFLAGS_OTHER})
target_link_libraries(gnvim PUBLIC ${GNVIM_LDFLAGS})
