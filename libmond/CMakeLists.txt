find_package(Lua 5.2 REQUIRED)

set(EXPORTED_INCLUDES
    include/mond/lua_read.hh
    include/mond/lua_write.hh
    include/mond/lua_types.hh
    include/mond/state.hh
    include/mond/focus.hh
    include/mond/tape.hh
    include/mond/metatable.hh
    include/mond/iterator.hh
    include/mond/function.hh
    include/mond/macros.h)

set(SRC
    ${EXPORTED_INCLUDES}
    src/mond/state.cc
    src/mond/focus.cc
    src/mond/tape.cc)

include_directories("${LUA_INCLUDE_DIR}")
include_directories(include/)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_library(mond_objs OBJECT ${SRC})

add_library(mond SHARED $<TARGET_OBJECTS:mond_objs>)
add_library(mond_static STATIC $<TARGET_OBJECTS:mond_objs>)

target_link_libraries(mond "${LUA_LIBRARIES}")
target_link_libraries(mond_static "${LUA_LIBRARIES}")
