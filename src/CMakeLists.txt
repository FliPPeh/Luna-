include_directories("${luna++_SOURCE_DIR}/libircclient/include/")
include_directories("${luna++_SOURCE_DIR}/libmond/include/")

set(SRC
    luna.h
    luna.cc
    luna_user.h
    luna_user.cc
    luna_script.h
    luna_script.cc
    proxies/luna_proxy.h
    proxies/luna_proxy.cc
    proxies/luna_channel_proxy.h
    proxies/luna_channel_proxy.cc
    proxies/luna_script_proxy.h
    proxies/luna_script_proxy.cc
    proxies/luna_user_proxy.h
    proxies/luna_user_proxy.cc
    client_base.h
    client_base.cc
    tokenbucket.h
    tokenbucket.cc
    logging.h
    logging.cc)

include_directories("${LUA_INCLUDE_DIR}")
include_directories("${luna++_SOURCE_DIR}/src")

add_executable(luna++ ${SRC})
target_link_libraries(luna++ ircclient mond)
