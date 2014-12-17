include_directories("${luna++_SOURCE_DIR}/libircclient/include/")
include_directories("${luna++_SOURCE_DIR}/libmond/include/")

set(SRC
    luna.hh
    luna.cc
    luna_user.hh
    luna_user.cc
    luna_extension.hh
    luna_extension.cc

    lua/luna_script.hh
    lua/luna_script.cc
    lua/proxies/luna_channel_proxy.hh
    lua/proxies/luna_channel_proxy.cc
    lua/proxies/luna_extension_proxy.hh
    lua/proxies/luna_extension_proxy.cc
    lua/proxies/luna_user_proxy.hh
    lua/proxies/luna_user_proxy.cc

    tokenbucket.hh
    tokenbucket.cc
    logging.hh
    logging.cc
    config.hh)

include_directories("${LUA_INCLUDE_DIR}")
include_directories("${luna++_SOURCE_DIR}/src")

add_executable(luna++ ${SRC})
target_link_libraries(luna++ ircclient mond)
