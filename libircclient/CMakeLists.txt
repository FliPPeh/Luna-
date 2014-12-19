find_package(Boost 1.54.0 REQUIRED system)
find_package(Threads      REQUIRED)
find_package(OpenSSL      REQUIRED)

add_definitions(-DBOOST_ASIO_HAS_MOVE=1)

set(EXPORTED_INCLUDES
    include/irc/client.hh
    include/irc/connection.hh
    include/irc/irc_core.hh
    include/irc/irc_utils.hh
    include/irc/irc_helpers.hh
    include/irc/irc_except.hh
    include/irc/environment.hh
    include/irc/channel.hh
    include/irc/channel_user.hh
    include/irc/macros.h)

set(SRC
    ${EXPORTED_INCLUDES}
    src/irc/client.cc
    src/irc/connection.cc
    src/irc/irc_core.cc
    src/irc/irc_utils.cc
    src/irc/irc_helpers.cc
    src/irc/environment.cc
    src/irc/channel.cc
    src/irc/channel_user.cc)

include_directories(${Boost_INCLUDE_DIR})
include_directories(${OpenSSL_INCLUDE_DIR})
include_directories(include/)

add_library(ircclient_objs OBJECT ${SRC})

add_library(ircclient SHARED $<TARGET_OBJECTS:ircclient_objs>)
add_library(ircclient_static STATIC $<TARGET_OBJECTS:ircclient_objs>)

target_link_libraries(ircclient ${OPENSSL_LIBRARIES}
                                ${Boost_LIBRARIES}
                                ${CMAKE_THREAD_LIBS_INIT})

target_link_libraries(ircclient_static ${OPENSSL_LIBRARIES}
                                       ${Boost_LIBRARIES}
                                       ${CMAKE_THREAD_LIBS_INIT})
