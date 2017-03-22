set(GNVIM_SOURCES
    app.cpp
    buffer.cpp
    main.cpp
    msgpack-adapter.cpp
    msgpack-rpc.cpp
    nvim-bridge.cpp
    view.cpp
    window.cpp
)

set(GNVIM_HEADERS
    app.h
    buffer.h
    defns.h
    msgpack-adapter.h
    msgpack-rpc.h
    nvim-bridge.h
    view.h
    window.h
)

#set(GNVIM_SIGNALS ${CMAKE_CURRENT_SOURCE_DIR}/gnvim-signals.py)
#set(NVIM_BRIDGE_CPP_IN ${CMAKE_CURRENT_SOURCE_DIR}/nvim-bridge.cpp.in)
#set(NVIM_BRIDGE_CPP ${CMAKE_CURRENT_BINARY_DIR}/nvim-bridge.cpp)
#set(NVIM_BRIDGE_H_IN ${CMAKE_CURRENT_SOURCE_DIR}/nvim-bridge.h.in)
#set(NVIM_BRIDGE_H ${CMAKE_CURRENT_BINARY_DIR}/nvim-bridge.h)
#add_custom_command(OUTPUT nvim-bridge.cpp
#    COMMAND ${GNVIM_SIGNALS} reg ${NVIM_BRIDGE_CPP_IN} ${NVIM_BRIDGE_CPP} 
#    MAIN_DEPENDENCY ${NVIM_BRIDGE_CPP_IN}
#    DEPENDS ${GNVIM_SIGNALS})
#add_custom_command(OUTPUT nvim-bridge.h
#    COMMAND ${GNVIM_SIGNALS} def ${NVIM_BRIDGE_H_IN} ${NVIM_BRIDGE_H} 
#    MAIN_DEPENDENCY ${NVIM_BRIDGE_H_IN}
#    DEPENDS ${GNVIM_SIGNALS})

add_executable(gnvim ${GNVIM_SOURCES} ${GNVIM_HEADERS})
target_include_directories(gnvim PUBLIC ${GNVIM_INCLUDE_DIRS})
#    ${CMAKE_CURRENT_BINARY_DIR})
target_compile_options(gnvim PUBLIC ${GNVIM_CFLAGS_OTHER})
target_link_libraries(gnvim PUBLIC ${GNVIM_LDFLAGS})
