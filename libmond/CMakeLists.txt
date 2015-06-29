set(EXPORTED_INCLUDES
    include/mond/lua_read.hh
    include/mond/lua_write.hh
    include/mond/lua_types.hh
    include/mond/state.hh
    include/mond/tape.hh
    include/mond/metatable.hh
    include/mond/iterator.hh
    include/mond/function.hh
    include/mond/macros.h)

set(SRC
    ${EXPORTED_INCLUDES})

include_directories(include/)

if(LUNA_LINK_STATIC)
    add_library(mond_static STATIC ${SRC})
    set_target_properties(mond_static PROPERTIES LINKER_LANGUAGE CXX)

else(LUNA_LINK_STATIC)
    add_library(mond STATIC ${SRC})
    set_target_properties(mond PROPERTIES LINKER_LANGUAGE CXX)

endif(LUNA_LINK_STATIC)

