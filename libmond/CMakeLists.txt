find_package(Lua 5.2 REQUIRED)

set(EXPORTED_INCLUDES
    include/lua_read.hh
    include/lua_write.hh
    include/lua_types.hh
    include/state.hh
    include/focus.hh
    include/tape.hh
    include/metatable.hh
    include/iterator.hh
    include/function.hh
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
