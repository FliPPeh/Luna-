include_directories("${luna++_SOURCE_DIR}/libircclient/include/")
include_directories("${luna++_SOURCE_DIR}/libmond/include/")

set(SRC
    luna.hh
    luna.cc
    luna_user.hh
    luna_user.cc
    luna_script.hh
    luna_script.cc
    proxies/luna_channel_proxy.hh
    proxies/luna_channel_proxy.cc
    proxies/luna_script_proxy.hh
    proxies/luna_script_proxy.cc
    proxies/luna_user_proxy.hh
    proxies/luna_user_proxy.cc
    client_base.hh
    client_base.cc
    tokenbucket.hh
    tokenbucket.cc
    logging.hh
    logging.cc
    config.hh)

include_directories("${LUA_INCLUDE_DIR}")
include_directories("${luna++_SOURCE_DIR}/src")

add_executable(luna++ ${SRC})
target_link_libraries(luna++ ircclient mond)
