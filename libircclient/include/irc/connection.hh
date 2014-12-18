/*
 * Copyright 2014 Lukas Niederbremer
 *
 * This file is part of libircclient.
 *
 * libircclient is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * libircclient is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with libircclient.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once
#ifndef LIBIRCCLIENT_CONNECTION_HH_INCLUDED
#define LIBIRCCLIENT_CONNECTION_HH_INCLUDED

#include "irc/macros.h"

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#include <string>
#include <functional>

namespace irc {

struct message;


namespace asio = boost::asio;

enum connection_flags : int {
    SSL = 0x01
};

/* \brief An asynchronous IRC connection using an external io_service */
class DLL_LOCAL async_connection {
public:
    using connect_handler = std::function<void (asio::ip::tcp::endpoint ep)>;
    using message_handler = std::function<void (message const&)>;
    using write_confirmation_handler = std::function<void (std::size_t)>;

    async_connection(asio::io_service& io_svc, int flags = 0);
    ~async_connection();

    async_connection(async_connection&&)            = default;
    async_connection& operator=(async_connection&&) = default;

    async_connection(async_connection const&)            = delete;
    async_connection& operator=(async_connection const&) = delete;

    void connect(
        std::string const& host,
        uint16_t port,
        connect_handler handler);

    void disconnect();

    void read_message(message_handler handler);
    void send_message(message const& msg, write_confirmation_handler handler);

    bool connected() const;
    bool ssl() const;

    void use_ssl(bool ssl);

    std::string server_host() const;
    std::string server_addr() const;
    uint16_t    server_port() const;

private:
    DLL_LOCAL void handle_resolve(
        boost::system::error_code const& err,
        boost::asio::ip::tcp::resolver::iterator endpoint_iter);

    DLL_LOCAL void handle_connect(
        boost::system::error_code const& err,
        boost::asio::ip::tcp::resolver::iterator endpoint_iter);

    DLL_LOCAL void handle_handshake(
        boost::system::error_code const& err,
        boost::asio::ip::tcp::resolver::iterator endpoint_iter);

private:
    asio::io_service* _io_service;
    asio::ssl::context _ssl;

    asio::ip::tcp::resolver _resolver;
    std::unique_ptr<asio::ssl::stream<asio::ip::tcp::socket>> _socket;

    asio::streambuf _buffer;

    connect_handler _connect_handler;

    bool _use_ssl;
};

}

#endif // defined LIBIRCCLIENT_CONNECTION_HH_INCLUDED
