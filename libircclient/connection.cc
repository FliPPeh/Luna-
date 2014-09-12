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

#include "connection.h"
#include "core.h"

#include <string>
#include <cstddef>
#include <cstring>
#include <functional>
#include <iostream>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

namespace irc {

using namespace std::placeholders;

async_connection::async_connection(asio::io_service& io_svc, int flags)
    : _io_service{&io_svc},
      _ssl{asio::ssl::context::sslv23_client},
      _resolver{*_io_service},
      _socket{nullptr},
      _buffer{},
      _connect_handler{nullptr},
      _use_ssl{(flags & connection_flags::SSL) > 0}
{
    _ssl.set_default_verify_paths();
    _ssl.set_options(asio::ssl::context::default_workarounds |
                     asio::ssl::context::verify_none);
}

async_connection::~async_connection()
{
    disconnect();
}

void async_connection::connect(
    const std::string& host,
    uint16_t port,
    connect_handler handler)
{
    if (_connect_handler) {
        throw connection_error{connection_error_type::connection_error,
            "already connected"};
    }

    asio::ip::tcp::resolver::query query{host, std::to_string(port)};

    _connect_handler = handler;

    _resolver.async_resolve(query,
        std::bind(&async_connection::handle_resolve, this, _1, _2));
}

void async_connection::disconnect()
{
    boost::system::error_code err;

    _socket->shutdown(err);
    _socket->next_layer().shutdown(
        asio::ip::tcp::socket::shutdown_type::shutdown_both, err);
    _socket->next_layer().close(err);
}


void async_connection::read_message(message_handler handler)
{
    auto cb_read =
        [this, handler] (boost::system::error_code const& err, std::size_t s) {
            if (err) {
                throw connection_error{connection_error_type::stream_error,
                    "read_message: " + err.message()};
            }

            std::string line;
            std::istream is(&_buffer);
            std::getline(is, line);

            if (line.back() == '\r') {
                line.erase(std::end(line) - 1);
            }

            if (not line.empty()) {
                handler(message_from_string(line));
            }
        };

    if (_use_ssl) {
        asio::async_read_until(*_socket, _buffer, '\n', cb_read);
    } else {
        asio::async_read_until(_socket->next_layer(), _buffer, '\n', cb_read);
    }
}

void async_connection::send_message(const message& msg)
{
    if (not connected()) {
        throw connection_error{connection_error_type::not_connected,
            "send_message"};
    }

    auto buf = asio::buffer(to_string(msg) + "\r\n");
    boost::system::error_code err;

    if (_use_ssl) {
        asio::write(*_socket, buf, err);
    } else {
        asio::write(_socket->next_layer(), buf, err);
    }

    if (err) {
        throw connection_error{connection_error_type::stream_error,
            "send_message: " + err.message()};
    }
}

bool async_connection::connected() const
{
    return _socket and _socket->lowest_layer().is_open();
}

bool async_connection::ssl() const
{
    return _use_ssl;
}


void async_connection::use_ssl(bool ssl)
{
    if (connected()) {
        throw connection_error{
            connection_error_type::cannot_change_security, "use_ssl"};
    }

    _use_ssl = ssl;
}


std::string async_connection::server_host() const
{
    if (not connected()) {
        throw connection_error{connection_error_type::not_connected,
            "server_host"};
    }

    asio::ip::tcp::resolver r(*_io_service);

    for (auto iter = r.resolve(_socket->lowest_layer().remote_endpoint());
              iter != asio::ip::tcp::resolver::iterator{};
            ++iter) {

        return iter->host_name();
    }

    return server_addr();
}

std::string async_connection::server_addr() const
{
    if (not connected()) {
        throw connection_error{connection_error_type::not_connected,
            "server_addr"};
    }

    std::string server_addr =
        _socket->lowest_layer().remote_endpoint().address().to_string();

    if (_socket->lowest_layer().remote_endpoint().address().is_v6()) {
        server_addr = "[" + server_addr + "]";
    }

    return server_addr;
}

uint16_t async_connection::server_port() const
{
    if (not connected()) {
        throw connection_error{connection_error_type::not_connected,
            "server_port"};
    }

    return _socket->lowest_layer().remote_endpoint().port();
}


void async_connection::handle_resolve(
    boost::system::error_code const& err,
    boost::asio::ip::tcp::resolver::iterator endpoint_iter)
{
    // Connect
    if (err) {
        throw connection_error{connection_error_type::lookup_error,
            "resolve: " + err.message()};
    }

    _socket.reset(new asio::ssl::stream<asio::ip::tcp::socket>(
        *_io_service, _ssl));

    asio::async_connect(_socket->lowest_layer(), endpoint_iter,
        std::bind(&async_connection::handle_connect, this, _1, _2));
}

void async_connection::handle_connect(
    boost::system::error_code const& err,
    boost::asio::ip::tcp::resolver::iterator endpoint_iter)
{
    // Maybe Handshake
    if (err) {
        throw connection_error{connection_error_type::connection_error,
            "connect: " + err.message()};
    }

    if (_use_ssl) {
        _socket->async_handshake(asio::ssl::stream_base::client,
            std::bind(
                &async_connection::handle_handshake, this, _1, endpoint_iter));
    } else {
        _connect_handler(endpoint_iter->endpoint());
        _connect_handler = nullptr;
    }
}

void async_connection::handle_handshake(
    boost::system::error_code const& err,
    boost::asio::ip::tcp::resolver::iterator endpoint_iter)
{
    if (err) {
        throw connection_error{connection_error_type::connection_error,
            "handshake: " + err.message()};
    }

    _connect_handler(endpoint_iter->endpoint());
    _connect_handler = nullptr;
}

}

