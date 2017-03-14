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

add_executable(example exampleappwindow.cc exampleapplication.cc
    examplemain.cc)
target_include_directories(example PUBLIC ${GNVIM_INCLUDE_DIRS})
target_compile_options(example PUBLIC ${GNVIM_CFLAGS_OTHER})
target_link_libraries(example PUBLIC ${GNVIM_LDFLAGS})
