find_package(Boost 1.54.0 REQUIRED system)
find_package(Threads      REQUIRED)
find_package(OpenSSL      REQUIRED)

add_definitions(-DBOOST_ASIO_HAS_MOVE=1)

set(EXPORTED_INCLUDES
    include/client.h
    include/connection.h
    include/irc_core.h
    include/irc_utils.h
    include/irc_helpers.h
    include/irc_except.h
    include/environment.h
    include/channel.h
    include/channel_user.h
    include/macros.h)

set(SRC
    ${EXPORTED_INCLUDES}
    client.cc
    connection.cc
    irc_core.cc
    irc_utils.cc
    irc_helpers.cc
    environment.cc
    channel.cc
    channel_user.cc)

include_directories(${Boost_INCLUDE_DIR})
include_directories(${OpenSSL_INCLUDE_DIR})
include_directories(include/)

add_library(ircclient SHARED ${SRC})

target_link_libraries(ircclient ${OPENSSL_LIBRARIES}
                                ${Boost_LIBRARIES}
                                ${CMAKE_THREAD_LIBS_INIT})
