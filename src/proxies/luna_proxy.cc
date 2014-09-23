/*
 * Copyright 2014 Lukas Niederbremer
 *
 * This file is part of Luna++.
 *
 * Luna++ is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Luna++ is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Luna++.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "luna_proxy.h"

#include "luna.h"

#include <irc_core.h>
#include <irc_except.h>

#include <mond.h>

#include <lua.hpp>

#include <cstddef>

#include <tuple>
#include <string>
#include <stdexcept>


luna_proxy::luna_proxy(luna& ref)
    : _ref{&ref}
{
}

std::tuple<std::time_t, std::time_t> luna_proxy::runtime_info() const
{
    return std::make_tuple(_ref->_started, _ref->_connected);
}

std::tuple< std::string, std::string > luna_proxy::user_info() const
{
    return std::make_tuple(_ref->nick(), _ref->user());
}

std::tuple< std::string, std::string, uint16_t > luna_proxy::server_info() const
{
    return std::make_tuple(
        _ref->server_host(),
        _ref->server_addr(),
        _ref->server_port());
}


std::tuple<
    std::size_t,
    std::size_t,
    std::size_t,
    std::size_t > luna_proxy::traffic_info() const
{
    return std::make_tuple(
        _ref->_bytes_sent,
        _ref->_bytes_sent_sess,
        _ref->_bytes_recvd,
        _ref->_bytes_recvd_sess);
}


int luna_proxy::send_message(lua_State* s)
{
    irc::message msg;

    std::string cmd = luaL_checkstring(s, 2);

    try {
        msg.command = irc::rfc1459_upper(cmd);
    } catch (irc::protocol_error const& pe) {
        std::throw_with_nested(mond::runtime_error{"invalid command: " + cmd});
    }

    int n = lua_gettop(s) - 2;

    for (int i = 3; i < (3 + n); ++i) {
        msg.args.push_back(luaL_checkstring(s, i));
    }

    _ref->send_message(msg);

    return 0;
}
