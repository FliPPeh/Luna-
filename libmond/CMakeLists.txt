find_package(Lua 5.2 REQUIRED)

set(EXPORTED_INCLUDES
    include/lua_read.h
    include/lua_write.h
    include/lua_types.h
    include/state.h
    include/focus.h
    include/tape.h
    include/metatable.h
    include/iterator.h
    include/function.h
    include/macros.h)

set(SRC
    ${EXPORTED_INCLUDES}
    state.cc
    focus.cc
    tape.cc)

include_directories("${LUA_INCLUDE_DIR}")
include_directories(include/)

add_library(mond SHARED ${SRC})

target_link_libraries(mond "${LUA_LIBRARIES}")
